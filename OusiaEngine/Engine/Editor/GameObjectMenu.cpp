#include "GameObjectMenu.h"
#include "Asset/AssetDatabase.h"
#include "Audio/AudioListener.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/Selection.h"
#include "Editor/Undo.h"
#include "Math/Mathf.h"
#include "Physics/BoxCollider.h"
#include "Physics/CapsuleCollider.h"
#include "Physics/SphereCollider.h"
#include "Render/Asset/Model.h"
#include "Render/Camera.h"
#include "Render/Light.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"
#include "UI/Button.h"
#include "UI/Canvas.h"
#include "UI/EventSystem/EventSystem.h"
#include "UI/EventSystem/StandaloneInputModule.h"
#include "UI/GraphicRaycaster.h"
#include "UI/Image.h"
#include "UI/RectTransform.h"
#include "UI/Text.h"

#include <functional>
#include <string>

#include <imgui/imgui.h>

using namespace ou;


namespace {

// Unity の PrimitiveType
enum class PrimitiveType {
	Sphere,
	Capsule,
	Cylinder,
	Cube,
	Plane,
	Quad,
};

const char *PrimitiveName(PrimitiveType type) {
	switch (type) {
	case PrimitiveType::Sphere: return "Sphere";
	case PrimitiveType::Capsule: return "Capsule";
	case PrimitiveType::Cylinder: return "Cylinder";
	case PrimitiveType::Cube: return "Cube";
	case PrimitiveType::Plane: return "Plane";
	case PrimitiveType::Quad: return "Quad";
	}
	return "";
}

// ---- 共通 ----

GameObject *CreateChild(Scene *scene, GameObject *parent, const std::string &name) {
	auto *gameObject = scene->CreateGameObject(name);
	if (parent) { gameObject->GetTransform()->SetParent(parent->GetTransform(), false); }
	return gameObject;
}

// シーンの中から、その Component を持つ GameObject を探す（親から順に）
template <typename T>
T *FindInChildren(GameObject *gameObject) {
	if (auto *found = gameObject->GetComponent<T>()) { return found; }
	for (auto *child : gameObject->GetTransform()->GetChildren()) {
		if (auto *found = FindInChildren<T>(child->GetGameObject())) { return found; }
	}
	return nullptr;
}

template <typename T>
T *FindInScene(Scene *scene) {
	for (auto *root : scene->GetRootGameObjects()) {
		if (auto *found = FindInChildren<T>(root)) { return found; }
	}
	return nullptr;
}

// 自分か親に Canvas があるか
Canvas *FindCanvasInParents(GameObject *gameObject) {
	for (auto *transform = gameObject ? gameObject->GetTransform() : nullptr; transform; transform = transform->GetParent()) {
		if (auto *canvas = transform->GetGameObject()->GetComponent<Canvas>()) { return canvas; }
	}
	return nullptr;
}


// ---- 3D Object（Unity と同じく Collider も付ける） ----

GameObject *CreatePrimitive(Scene *scene, GameObject *parent, PrimitiveType type) {
	const std::string name = PrimitiveName(type);
	auto *gameObject = CreateChild(scene, parent, name);

	// 形は EngineAssets/Primitives/*.obj（Unity と同じ大きさ）。アセットなので、シーンに保存できる
	const auto path = "EngineAssets/Primitives/" + name + ".obj";
	if (const auto model = AssetDatabase::LoadAssetAtPath<Model>(path)) {
		for (const auto &node : model->GetNodes()) {
			if (!node.mesh) { continue; }
			auto *renderer = gameObject->AddComponent<MeshRenderer>();
			renderer->SetMesh(node.mesh);
			renderer->SetSharedMaterials(node.materials);
			break;
		}
	} else {
		Log::Error("基本の形が見つかりません: {}", path);
	}

	switch (type) {
	case PrimitiveType::Cube:
		gameObject->AddComponent<BoxCollider>();
		break;
	case PrimitiveType::Sphere:
		gameObject->AddComponent<SphereCollider>();
		break;
	case PrimitiveType::Capsule:
	case PrimitiveType::Cylinder:
		// Unity も Cylinder には CapsuleCollider を付ける
		gameObject->AddComponent<CapsuleCollider>();
		break;
	case PrimitiveType::Plane: {
		// Unity は MeshCollider。ないので、上の面が y = 0 に来る薄い箱にする
		auto *collider = gameObject->AddComponent<BoxCollider>();
		collider->SetSize({10.0f, 0.02f, 10.0f});
		collider->SetCenter({0.0f, -0.01f, 0.0f});
		break;
	}
	case PrimitiveType::Quad:
		// Unity は MeshCollider。ないので付けない
		break;
	}
	return gameObject;
}


// ---- Camera・Light ----

GameObject *CreateCamera(Scene *scene, GameObject *parent) {
	auto *gameObject = CreateChild(scene, parent, "Camera");
	gameObject->AddComponent<Camera>();
	gameObject->AddComponent<AudioListener>();
	gameObject->GetTransform()->SetLocalPosition({0.0f, 1.0f, -10.0f});
	return gameObject;
}

GameObject *CreateLight(Scene *scene, GameObject *parent, LightType type) {
	const char *name = type == LightType::Directional ? "Directional Light" : type == LightType::Point ? "Point Light" : "Spot Light";
	auto *gameObject = CreateChild(scene, parent, name);
	gameObject->AddComponent<Light>()->SetType(type);

	// Unity と同じ向き（Directional は斜め上から、Spot は真下を向く）
	auto *transform = gameObject->GetTransform();
	if (type == LightType::Directional) {
		transform->SetLocalPosition({0.0f, 3.0f, 0.0f});
		transform->SetLocalEulerAngles(Vector3(50.0f, -30.0f, 0.0f) * Mathf::Deg2Rad);
	} else if (type == LightType::Spot) {
		transform->SetLocalPosition({0.0f, 3.0f, 0.0f});
		transform->SetLocalEulerAngles(Vector3(90.0f, 0.0f, 0.0f) * Mathf::Deg2Rad);
	}
	return gameObject;
}


// ---- UI（Unity と同じく、Canvas がなければ Canvas と EventSystem も作る） ----

GameObject *CreateEventSystemIfMissing(Scene *scene) {
	if (FindInScene<EventSystem>(scene)) { return nullptr; }

	auto *gameObject = scene->CreateGameObject("EventSystem");
	gameObject->AddComponent<EventSystem>();
	gameObject->AddComponent<StandaloneInputModule>();
	return gameObject;
}

GameObject *CreateCanvas(Scene *scene, GameObject *parent) {
	auto *gameObject = CreateChild(scene, parent, "Canvas");
	gameObject->AddComponent<RectTransform>();
	gameObject->AddComponent<Canvas>();
	gameObject->AddComponent<GraphicRaycaster>();
	CreateEventSystemIfMissing(scene);
	return gameObject;
}

// UI の部品の親: 選んでいるものが Canvas の中ならそこ、なければシーンの Canvas（なければ作る）
GameObject *FindUiParent(Scene *scene, GameObject *parent) {
	if (FindCanvasInParents(parent)) { return parent; }
	if (auto *canvas = FindInScene<Canvas>(scene)) { return canvas->GetGameObject(); }
	return CreateCanvas(scene, nullptr);
}

GameObject *CreateUiObject(Scene *scene, GameObject *parent, const std::string &name, const Vector2 &size) {
	auto *gameObject = CreateChild(scene, FindUiParent(scene, parent), name);
	gameObject->AddComponent<RectTransform>()->SetSizeDelta(size);
	return gameObject;
}

GameObject *CreateText(Scene *scene, GameObject *parent) {
	auto *gameObject = CreateUiObject(scene, parent, "Text", {160.0f, 30.0f});
	auto *text = gameObject->AddComponent<Text>();
	text->SetText("New Text");
	text->SetFontSize(24.0f);
	return gameObject;
}

GameObject *CreateImage(Scene *scene, GameObject *parent) {
	auto *gameObject = CreateUiObject(scene, parent, "Image", {100.0f, 100.0f});
	gameObject->AddComponent<Image>();
	return gameObject;
}

GameObject *CreateButton(Scene *scene, GameObject *parent) {
	auto *gameObject = CreateUiObject(scene, parent, "Button", {160.0f, 30.0f});
	gameObject->AddComponent<Image>();
	gameObject->AddComponent<Button>();

	// 子の Text はボタンいっぱいに広げる
	auto *label = scene->CreateGameObject("Text");
	label->GetTransform()->SetParent(gameObject->GetTransform(), false);
	auto *rect = label->AddComponent<RectTransform>();
	rect->SetAnchorMin({0.0f, 0.0f});
	rect->SetAnchorMax({1.0f, 1.0f});
	rect->SetSizeDelta({0.0f, 0.0f});
	auto *text = label->AddComponent<Text>();
	text->SetText("Button");
	text->SetFontSize(20.0f);
	text->SetAlignment(TextAlignment::Center);
	text->SetColor({0.2f, 0.2f, 0.2f, 1.0f});
	return gameObject;
}

} // namespace



void ou::GameObjectMenu::DrawItems(Scene *scene, GameObject *parent) {
	if (!scene) { return; }

	GameObject *created = nullptr;
	const auto item = [&](const char *label, const std::function<GameObject *()> &create) {
		if (ImGui::MenuItem(label)) { created = create(); }
	};

	item("Create Empty", [&] { return CreateChild(scene, parent, "GameObject"); });

	if (ImGui::BeginMenu("3D Object")) {
		for (const auto type : {PrimitiveType::Cube, PrimitiveType::Sphere, PrimitiveType::Capsule, PrimitiveType::Cylinder, PrimitiveType::Plane, PrimitiveType::Quad}) {
			item(PrimitiveName(type), [&] { return CreatePrimitive(scene, parent, type); });
		}
		ImGui::EndMenu();
	}

	item("Camera", [&] { return CreateCamera(scene, parent); });

	if (ImGui::BeginMenu("Light")) {
		item("Directional Light", [&] { return CreateLight(scene, parent, LightType::Directional); });
		item("Point Light", [&] { return CreateLight(scene, parent, LightType::Point); });
		item("Spot Light", [&] { return CreateLight(scene, parent, LightType::Spot); });
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("UI")) {
		item("Canvas", [&] { return CreateCanvas(scene, parent); });
		item("Text", [&] { return CreateText(scene, parent); });
		item("Image", [&] { return CreateImage(scene, parent); });
		item("Button", [&] { return CreateButton(scene, parent); });
		ImGui::Separator();
		item("Event System", [&] { return CreateEventSystemIfMissing(scene); });
		ImGui::EndMenu();
	}

	if (created) {
		Selection::SetActiveGameObject(created);
		Undo::SetCurrentGroupName("Create " + created->GetName());
		EditorSceneManager::MarkSceneDirty(scene);
	}
}
