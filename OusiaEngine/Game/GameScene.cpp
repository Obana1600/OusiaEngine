#include "GameScene.h"
#include "Asset/AssetDatabase.h"
#include "Core/Application.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemRenderer.h"
#include "Platform/Paths.h"
#include "Render/Asset/Material.h"
#include "Render/Camera.h"
#include "Render/LineRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Serialize/SceneSerializer.h"

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using namespace ou;


namespace {

namespace fs = std::filesystem;

using Json = nlohmann::ordered_json;

int sFailCount = 0;

void CheckTrue(const char *name, bool ok) {
	if (!ok) { sFailCount++; }
	Log::Info("TEST {} {}", ok ? "OK  " : "FAIL", name);
}

fs::path ScenePath() {
	return FindAssetsFolder().value_or(fs::path()) / "_test" / "ParticleTest.scene";
}

Json *FindComponentJson(Json &root, std::string_view objectName, std::string_view type) {
	for (auto &gameObject : root["gameObjects"]) {
		if (gameObject["name"].get<std::string>() != objectName) { continue; }
		for (auto &component : gameObject["components"]) {
			if (component["$type"].get<std::string>() == type) { return &component; }
		}
	}
	return nullptr;
}

template <typename T>
T *FindComponent(Scene &scene, std::string_view objectName) {
	auto *object = scene.Find(objectName);
	return object ? object->GetComponent<T>() : nullptr;
}

ParticleSystem *CreateParticles(Scene &scene, std::string_view name, const Vector3 &position) {
	auto *object = scene.CreateGameObject(name);
	object->GetTransform()->SetLocalPosition(position);
	// ParticleSystemRenderer の Awake が ParticleSystem を付ける
	object->AddComponent<ParticleSystemRenderer>();
	return object->GetComponent<ParticleSystem>();
}

void BuildParticleScene(Scene &scene) {
	auto *camera = scene.CreateGameObject("Main Camera");
	camera->AddComponent<Camera>();
	camera->GetTransform()->SetLocalPosition({0.0f, 1.2f, -6.0f});

	// 左: 上に噴き出す。アルファブレンド・色と大きさの変化・バースト
	auto *fountain = CreateParticles(scene, "Fountain", {-1.8f, 0.0f, 0.0f});
	auto &main = fountain->GetMain();
	main.duration = 3.0f;
	main.startLifetime = {1.0f, 2.0f};
	main.startSpeed = {2.0f, 4.0f};
	main.startSize = {0.1f, 0.3f};
	main.startColor = {Color{1.0f, 0.5f, 0.1f, 1.0f}, Color{1.0f, 1.0f, 0.2f, 1.0f}};
	main.gravityModifier = 0.5f;
	main.maxParticles = 500;
	main.simulationSpace = ParticleSystem::SimulationSpace::World;
	fountain->GetEmission().rateOverTime = 60.0f;
	fountain->GetEmission().bursts = {{0.0f, 50}, {0.5f, 20}};
	fountain->GetShape().shapeType = ParticleSystem::ShapeType::Cone;
	fountain->GetShape().angle = 20.0f;
	fountain->GetShape().radius = 0.2f;
	fountain->GetColorOverLifetime().enabled = true;
	fountain->GetColorOverLifetime().end = {1.0f, 0.0f, 0.0f, 0.0f};
	fountain->GetSizeOverLifetime().enabled = true;
	fountain->GetSizeOverLifetime().end = 0.2f;
	auto *fountainRenderer = fountain->GetGameObject()->GetComponent<ParticleSystemRenderer>();
	fountainRenderer->SetBlendMode(ParticleSystemRenderer::BlendMode::Alpha);
	fountainRenderer->SetSortMode(ParticleSystemRenderer::SortMode::Distance);

	// 右: ほぼ既定のまま（加算）
	auto *sparks = CreateParticles(scene, "Sparks", {1.8f, 0.5f, 0.0f});
	sparks->GetMain().startLifetime = 1.0f;
	sparks->GetMain().startSpeed = 1.5f;
	sparks->GetMain().startSize = 0.15f;
	sparks->GetEmission().rateOverTime = 40.0f;

	// 自動では出ないもの
	auto *stopped = CreateParticles(scene, "Stopped", {0.0f, -5.0f, 0.0f});
	stopped->GetMain().playOnAwake = false;
	stopped->Stop();
	stopped->Clear();

	// 輪になった線
	auto *ringObject = scene.CreateGameObject("Ring");
	ringObject->GetTransform()->SetLocalPosition({0.0f, 2.2f, 0.0f});
	auto *ring = ringObject->AddComponent<LineRenderer>();
	const std::vector<Vector3> points = {{-0.6f, -0.6f, 0.0f}, {0.6f, -0.6f, 0.0f}, {0.6f, 0.6f, 0.0f}, {-0.6f, 0.6f, 0.0f}};
	ring->SetPositions(points);
	ring->SetStartWidth(0.05f);
	ring->SetEndWidth(0.2f);
	ring->SetWidthMultiplier(1.5f);
	ring->SetStartColor({0.2f, 1.0f, 0.4f, 1.0f});
	ring->SetEndColor({0.2f, 0.4f, 1.0f, 1.0f});
	ring->SetUseWorldSpace(false);
	ring->SetLoop(true);
}

bool SaveParticleScene() {
	Scene temp("Temp", -2);
	BuildParticleScene(temp);

	Log::Info("TEST ここから何も出なければ正しい（既定のマテリアルとコピーは、警告なしで null と書く）");
	auto root = SceneSerializer::ToJson(temp);
	Log::Info("TEST ここまで");

	const auto *system = FindComponentJson(root, "Fountain", "ParticleSystem");
	const auto *renderer = FindComponentJson(root, "Fountain", "ParticleSystemRenderer");
	const auto *sparksSystem = FindComponentJson(root, "Sparks", "ParticleSystem");
	const auto *sparksRenderer = FindComponentJson(root, "Sparks", "ParticleSystemRenderer");
	const auto *stoppedSystem = FindComponentJson(root, "Stopped", "ParticleSystem");
	const auto *ring = FindComponentJson(root, "Ring", "LineRenderer");
	if (!system || !renderer || !sparksSystem || !sparksRenderer || !stoppedSystem || !ring) {
		CheckTrue("Component が登録されていて、書かれる", false);
		return false;
	}
	Log::Info("TEST ParticleSystem:\n{}", system->dump(1, '\t'));
	Log::Info("TEST ParticleSystemRenderer: {}", renderer->dump());
	Log::Info("TEST LineRenderer: {}", ring->dump());

	const auto &main = (*system)["main"];
	CheckTrue("main: 数値と bool", main["duration"] == 3.0 && main["looping"] == true && main["playOnAwake"] == true && main["gravityModifier"] == 0.5);
	CheckTrue("main: 範囲は min と max", main["startLifetime"].dump() == "{\"min\":1.0,\"max\":2.0}" && main["startSize"].dump() == "{\"min\":0.1,\"max\":0.3}" &&
											 main["startColor"]["min"].dump() == "[1.0,0.5,0.1,1.0]" && main["startColor"]["max"].dump() == "[1.0,1.0,0.2,1.0]");
	CheckTrue("main: uint32_t と enum", main["maxParticles"].dump() == "500" && main["simulationSpace"] == "World");
	CheckTrue("emission: バーストは構造体の配列", (*system)["emission"]["rateOverTime"] == 60.0 &&
													 (*system)["emission"]["bursts"].dump() == "[{\"time\":0.0,\"count\":50},{\"time\":0.5,\"count\":20}]");
	CheckTrue("shape", (*system)["shape"]["shapeType"] == "Cone" && (*system)["shape"]["angle"] == 20.0 && (*system)["shape"]["radius"].dump() == "0.2");
	CheckTrue("colorOverLifetime / sizeOverLifetime", (*system)["colorOverLifetime"]["enabled"] == true && (*system)["colorOverLifetime"]["end"].dump() == "[1.0,0.0,0.0,0.0]" &&
														   (*system)["sizeOverLifetime"]["enabled"] == true && (*system)["sizeOverLifetime"]["end"].dump() == "0.2");
	CheckTrue("既定のモジュール", (*sparksSystem)["emission"]["bursts"].empty() && (*sparksSystem)["shape"]["shapeType"] == "Sphere" &&
									  (*sparksSystem)["colorOverLifetime"]["enabled"] == false && (*stoppedSystem)["main"]["playOnAwake"] == false);
	CheckTrue("ParticleSystemRenderer: ブレンドと並べ替え", (*renderer)["blendMode"] == "Alpha" && (*renderer)["sortMode"] == "Distance" &&
																(*sparksRenderer)["blendMode"] == "Additive" && (*sparksRenderer)["sortMode"] == "None");
	CheckTrue("ParticleSystemRenderer: 既定のマテリアルとコピーは null", (*renderer)["materials"].dump() == "[null]" && (*sparksRenderer)["materials"].dump() == "[null]");
	CheckTrue("materials と enabled は 1 回だけ書く", renderer->size() == 6 && (*renderer)["enabled"] == true);
	CheckTrue("LineRenderer: 点の配列", (*ring)["positions"].size() == 4 && (*ring)["positions"][2].dump() == "[0.6,0.6,0.0]");
	CheckTrue("LineRenderer: 値", (*ring)["startWidth"].dump() == "0.05" && (*ring)["endWidth"].dump() == "0.2" && (*ring)["widthMultiplier"] == 1.5 &&
									  (*ring)["startColor"].dump() == "[0.2,1.0,0.4,1.0]" && (*ring)["endColor"].dump() == "[0.2,0.4,1.0,1.0]" &&
									  (*ring)["useWorldSpace"] == false && (*ring)["loop"] == true && (*ring)["materials"].dump() == "[null]");

	// --- 読んで、すぐ書き直す
	{
		Scene dest("Dest", -3);
		CheckTrue("読み込める", SceneSerializer::FromJson(dest, root));
		const auto resaved = SceneSerializer::ToJson(dest);
		CheckTrue("保存 → 読み込み → 保存で同じ json", resaved == root);
		if (resaved != root) {
			Log::Info("TEST 2 回目:\n{}", resaved.dump(1, '\t'));
		}
	}

	// --- 形の違う値
	{
		auto broken = root;
		auto &s = *FindComponentJson(broken, "Fountain", "ParticleSystem");
		s["main"]["maxParticles"] = -5;
		s["main"]["simulationSpace"] = "Nope";
		s["main"]["startLifetime"] = 3;
		s["emission"]["bursts"] = Json::parse(R"([{ "time": 1, "count": -1 }, { "time": 2, "count": 5000000000 }, { "count": 7 }])");
		s["shape"] = "x";
		(*FindComponentJson(broken, "Fountain", "ParticleSystemRenderer"))["blendMode"] = "Count";
		(*FindComponentJson(broken, "Sparks", "ParticleSystemRenderer"))["sortMode"] = 3;
		(*FindComponentJson(broken, "Ring", "LineRenderer"))["positions"] = Json::parse("[[1, 2, 3], [1, 2]]");

		Scene dest("Broken", -4);
		Log::Info("TEST ここから警告が 9 行出れば正しい（形の違う値）");
		const auto loaded = SceneSerializer::FromJson(dest, broken);
		Log::Info("TEST ここまで");
		auto *systemLoaded = FindComponent<ParticleSystem>(dest, "Fountain");
		auto *rendererLoaded = FindComponent<ParticleSystemRenderer>(dest, "Fountain");
		auto *sparksRendererLoaded = FindComponent<ParticleSystemRenderer>(dest, "Sparks");
		auto *ringLoaded = FindComponent<LineRenderer>(dest, "Ring");
		if (!loaded || !systemLoaded || !rendererLoaded || !sparksRendererLoaded || !ringLoaded) {
			CheckTrue("形の違う値でも読み込みは続く", false);
			return false;
		}
		CheckTrue("負の maxParticles: 既定の 1000 のまま", systemLoaded->GetMain().maxParticles == 1000);
		CheckTrue("違う enum・数値の範囲: 既定のまま", systemLoaded->GetMain().simulationSpace == ParticleSystem::SimulationSpace::Local &&
														  systemLoaded->GetMain().startLifetime.min == 5.0f && systemLoaded->GetMain().startLifetime.max == 5.0f);
		const auto &bursts = systemLoaded->GetEmission().bursts;
		CheckTrue("バースト: 負や大きすぎる count は既定の 30、ほかの値は読む", bursts.size() == 3 && bursts[0].time == 1.0f && bursts[0].count == 30 &&
																					bursts[1].time == 2.0f && bursts[1].count == 30 && bursts[2].time == 0.0f && bursts[2].count == 7);
		CheckTrue("文字列の shape: 既定のまま", systemLoaded->GetShape().shapeType == ParticleSystem::ShapeType::Sphere && systemLoaded->GetShape().angle == 25.0f);
		CheckTrue("ほかのモジュールは読む", systemLoaded->GetMain().duration == 3.0f && systemLoaded->GetColorOverLifetime().enabled);
		CheckTrue("blendMode の Count・数値の sortMode: 既定のまま", rendererLoaded->GetBlendMode() == ParticleSystemRenderer::BlendMode::Additive &&
																	   sparksRendererLoaded->GetSortMode() == ParticleSystemRenderer::SortMode::None);
		CheckTrue("線の点: 個数の違う要素だけ読まない", ringLoaded->GetPositionCount() == 2 && ringLoaded->GetPosition(0).z == 3.0f);
	}

	CheckTrue("シーンを保存", SceneSerializer::Save(temp, ScenePath()));
	return true;
}


// ファイルから読み込んだシーンを確かめる
class Driver final: public Component {
public:
	int tick = 0;
	bool checked = false;

	void Update() override {
		tick++;
		if (tick == 2) { SceneManager::LoadScene("ParticleTest"); }

		if (!checked && Time::GetTime() >= 2.5f) {
			checked = true;
			Check();
		}
		if (Time::GetTime() >= 6.0f) {
			std::error_code ec;
			fs::remove(ScenePath(), ec);
			Application::Quit();
		}
	}

private:
	static void Check() {
		auto *scene = SceneManager::GetActiveScene();
		if (!scene || scene->GetName() != "ParticleTest") {
			CheckTrue("ファイルのシーンに切り替わる", false);
			return;
		}

		auto *fountain = FindComponent<ParticleSystem>(*scene, "Fountain");
		auto *fountainRenderer = FindComponent<ParticleSystemRenderer>(*scene, "Fountain");
		auto *sparks = FindComponent<ParticleSystem>(*scene, "Sparks");
		auto *sparksRenderer = FindComponent<ParticleSystemRenderer>(*scene, "Sparks");
		auto *stopped = FindComponent<ParticleSystem>(*scene, "Stopped");
		auto *stoppedRenderer = FindComponent<ParticleSystemRenderer>(*scene, "Stopped");
		auto *ring = FindComponent<LineRenderer>(*scene, "Ring");
		if (!fountain || !fountainRenderer || !sparks || !sparksRenderer || !stopped || !stoppedRenderer || !ring) {
			CheckTrue("Component が見つかる", false);
			return;
		}

		Log::Info("TEST パーティクルの数: Fountain {}  Sparks {}  Stopped {}", fountain->GetParticles().size(), sparks->GetParticles().size(), stopped->GetParticles().size());
		CheckTrue("読み込み: playOnAwake で出ている", fountain->IsPlaying() && !fountain->GetParticles().empty() && sparks->IsPlaying() && !sparks->GetParticles().empty());
		CheckTrue("読み込み: maxParticles を超えない", fountain->GetParticles().size() <= 500 && fountain->GetMain().maxParticles == 500);
		CheckTrue("読み込み: playOnAwake = false は出ない", !stopped->IsPlaying() && stopped->GetParticles().empty());
		CheckTrue("読み込み: モジュールの値", fountain->GetMain().startSpeed.min == 2.0f && fountain->GetMain().startSpeed.max == 4.0f &&
												  fountain->GetEmission().bursts.size() == 2 && fountain->GetEmission().bursts[1].count == 20 &&
												  fountain->GetShape().shapeType == ParticleSystem::ShapeType::Cone && fountain->GetSizeOverLifetime().enabled);

		CheckTrue("読み込み: ブレンドと並べ替え", fountainRenderer->GetBlendMode() == ParticleSystemRenderer::BlendMode::Alpha &&
													  fountainRenderer->GetSortMode() == ParticleSystemRenderer::SortMode::Distance);
		CheckTrue("読み込み: Awake が既定のマテリアルを入れ直す", sparksRenderer->GetSharedMaterial() != nullptr && sparksRenderer->GetSharedMaterial() == stoppedRenderer->GetSharedMaterial());
		CheckTrue("読み込み: 加算でないものは、ブレンドを上書きしたコピーになる", fountainRenderer->GetSharedMaterial() != nullptr &&
																					  fountainRenderer->GetSharedMaterial() != sparksRenderer->GetSharedMaterial());

		CheckTrue("読み込み: 線の点と値", ring->GetPositionCount() == 4 && ring->GetPosition(2).x == 0.6f && ring->GetLoop() && !ring->GetUseWorldSpace() &&
											  ring->GetStartWidth() == 0.05f && ring->GetEndWidth() == 0.2f && ring->GetWidthMultiplier() == 1.5f);
		CheckTrue("読み込み: 線のマテリアルも既定のものが入る", ring->GetSharedMaterial() != nullptr);

		Log::Info("TEST 失敗 {} 件", sFailCount);
	}
};

} // namespace



void ou::BuildGameScene(Scene &scene) {
	if (SaveParticleScene()) {
		SceneManager::AddScene("Assets/_test/ParticleTest.scene");
	}

	auto *driver = scene.CreateGameObject("Driver");
	driver->AddComponent<Driver>();
	driver->AddComponent<QuitAfterFrames>();
	Object::DontDestroyOnLoad(driver);
}


void ou::QuitAfterFrames::Update() {
	// Driver が終わらせる。止まったときの保険
	if (Time::GetTime() >= 12.0f) { Application::Quit(); }
}
