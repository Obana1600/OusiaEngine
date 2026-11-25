#include "ScriptBindings.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Script/ScriptBindingsInternal.h"

#include <cstdint>
#include <format>
#include <new>
#include <string>
#include <vector>

#include <angelscript.h>

using namespace ou;
using namespace ou::ScriptBindingsInternal;


namespace {

// ---- Scene（Unity の SceneManagement.Scene） ----
// C++ の Scene はシーンを読み替えると消えるので、スクリプトには中身を写した値で渡す

struct ScriptScene {
	std::string name;
	std::string path;
	int buildIndex = -1;
};

ScriptScene MakeScriptScene(const Scene *scene) {
	if (!scene) { return {}; }
	return {scene->GetName(), scene->GetPath(), scene->GetBuildIndex()};
}

void ConstructScene(ScriptScene *self) {
	new (self) ScriptScene();
}

void CopyConstructScene(const ScriptScene &other, ScriptScene *self) {
	new (self) ScriptScene(other);
}

void DestructScene(ScriptScene *self) {
	self->~ScriptScene();
}

ScriptScene &AssignScene(const ScriptScene &other, ScriptScene *self) {
	*self = other;
	return *self;
}

bool EqualsScene(const ScriptScene &other, const ScriptScene *self) {
	return self->path == other.path && self->name == other.name && self->buildIndex == other.buildIndex;
}

std::string GetSceneName(const ScriptScene *self) { return self->name; }
std::string GetScenePath(const ScriptScene *self) { return self->path; }
int GetSceneBuildIndex(const ScriptScene *self) { return self->buildIndex; }

// 名前が空なら、どのシーンも指していない（Unity の default(Scene)）
bool IsSceneValid(const ScriptScene *self) { return !self->name.empty(); }

// DontDestroyOnLoad のシーンは SceneManager の friend の ScriptBindings しか引けないので、登録のときに関数を入れておく
Scene *(*findDontDestroyOnLoadScene)() = nullptr;

// 写した値から、今あるシーン（今のシーンと DontDestroyOnLoad のシーン）を引き直す。なければ null
Scene *FindScene(const ScriptScene &scene) {
	if (scene.name.empty()) { return nullptr; }
	for (auto *candidate : {SceneManager::GetActiveScene(), findDontDestroyOnLoadScene()}) {
		if (candidate && candidate->GetName() == scene.name && candidate->GetPath() == scene.path) { return candidate; }
	}
	return nullptr;
}

// Unity と同じく、読み込まれていないシーンなら例外にする
Scene *ResolveScene(const ScriptScene &scene) {
	auto *found = FindScene(scene);
	if (!found) { ThrowException(std::format("ArgumentException: シーン {} は読み込まれていません", scene.name)); }
	return found;
}

bool IsSceneLoaded(const ScriptScene *self) { return FindScene(*self) != nullptr; }

int GetRootCount(const ScriptScene *self) {
	auto *scene = ResolveScene(*self);
	return scene ? static_cast<int>(scene->GetRootGameObjects().size()) : 0;
}

CScriptArray *GetRootGameObjects(const ScriptScene *self) {
	auto *scene = ResolveScene(*self);
	return MakeHandleArray(scene ? scene->GetRootGameObjects() : std::vector<GameObject *>{}, "array<GameObject@>");
}


// ---- GameObject.scene ----

ScriptScene GetGameObjectScene(ObjectHandle *self) {
	auto *gameObject = Resolve<GameObject>(self);
	return gameObject ? MakeScriptScene(gameObject->GetScene()) : ScriptScene{};
}


// ---- SceneManager ----

// 読み込みは今のフレームの終わり（Unity と同じく、呼んだすぐあとはまだ前のシーン）
void LoadSceneByName(const std::string &sceneName) {
	SceneManager::LoadScene(sceneName);
}

void LoadSceneByIndex(int sceneBuildIndex) {
	SceneManager::LoadScene(static_cast<int32_t>(sceneBuildIndex));
}

ScriptScene GetActiveScene() {
	return MakeScriptScene(SceneManager::GetActiveScene());
}

int GetSceneCountInBuildSettings() {
	return static_cast<int>(SceneManager::GetSceneCountInBuildSettings());
}

// Unity と同じくルートの GameObject だけ動かせる（C++ がログに出す）
void MoveGameObjectToScene(ObjectHandle *gameObject, const ScriptScene &scene) {
	if (!gameObject) {
		ThrowException("ArgumentException: gameObject が null です");
		return;
	}
	auto *resolved = Resolve<GameObject>(gameObject);
	auto *target = resolved ? ResolveScene(scene) : nullptr;
	if (target) { SceneManager::MoveGameObjectToScene(resolved, target); }
}

} // namespace



Scene *ou::ScriptBindings::FindDontDestroyOnLoadScene() {
	return SceneManager::GetDontDestroyOnLoadScene();
}


void ou::ScriptBindings::RegisterSceneTypes(asIScriptEngine *engine) {
	Check(engine->RegisterObjectType("Scene", sizeof(ScriptScene), asOBJ_VALUE | asGetTypeTraits<ScriptScene>()), "Scene");
}


void ou::ScriptBindings::RegisterSceneManager(asIScriptEngine *engine) {
	findDontDestroyOnLoadScene = &FindDontDestroyOnLoadScene;

	const auto method = [&](const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterObjectMethod("Scene", declaration, function, asCALL_CDECL_OBJLAST), declaration);
	};
	const auto behaviour = [&](asEBehaviours kind, const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterObjectBehaviour("Scene", kind, declaration, function, asCALL_CDECL_OBJLAST), declaration);
	};

	// ---- Scene ----
	behaviour(asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructScene));
	behaviour(asBEHAVE_CONSTRUCT, "void f(const Scene &in other)", asFUNCTION(CopyConstructScene));
	behaviour(asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructScene));
	method("Scene &opAssign(const Scene &in other)", asFUNCTION(AssignScene));
	method("bool opEquals(const Scene &in other) const", asFUNCTION(EqualsScene));
	method("string get_name() const property", asFUNCTION(GetSceneName));
	method("string get_path() const property", asFUNCTION(GetScenePath));
	method("int get_buildIndex() const property", asFUNCTION(GetSceneBuildIndex));
	method("bool IsValid() const", asFUNCTION(IsSceneValid));
	method("bool get_isLoaded() const property", asFUNCTION(IsSceneLoaded));
	method("int get_rootCount() const property", asFUNCTION(GetRootCount));
	method("array<GameObject@>@ GetRootGameObjects() const", asFUNCTION(GetRootGameObjects));

	// ---- GameObject.scene ----
	{
		const TypeRegistrar g{engine, "GameObject"};
		g.Method("Scene get_scene() const property", asFUNCTION(GetGameObjectScene));
	}

	// ---- SceneManager ----
	engine->SetDefaultNamespace("SceneManager");

	const auto global = [&](const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterGlobalFunction(declaration, function, asCALL_CDECL), declaration);
	};
	global("void LoadScene(const string &in sceneName)", asFUNCTION(LoadSceneByName));
	global("void LoadScene(int sceneBuildIndex)", asFUNCTION(LoadSceneByIndex));
	global("Scene GetActiveScene()", asFUNCTION(GetActiveScene));
	global("int get_sceneCountInBuildSettings() property", asFUNCTION(GetSceneCountInBuildSettings));
	global("void MoveGameObjectToScene(GameObject@+ go, const Scene &in scene)", asFUNCTION(MoveGameObjectToScene));

	engine->SetDefaultNamespace("");
}
