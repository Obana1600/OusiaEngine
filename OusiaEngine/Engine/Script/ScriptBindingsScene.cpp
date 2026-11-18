#include "ScriptBindings.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Script/ScriptBindingsInternal.h"

#include <cstdint>
#include <new>
#include <string>

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

} // namespace



void ou::ScriptBindings::RegisterSceneTypes(asIScriptEngine *engine) {
	Check(engine->RegisterObjectType("Scene", sizeof(ScriptScene), asOBJ_VALUE | asGetTypeTraits<ScriptScene>()), "Scene");
}


void ou::ScriptBindings::RegisterSceneManager(asIScriptEngine *engine) {
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

	// ---- SceneManager ----
	engine->SetDefaultNamespace("SceneManager");

	const auto global = [&](const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterGlobalFunction(declaration, function, asCALL_CDECL), declaration);
	};
	global("void LoadScene(const string &in sceneName)", asFUNCTION(LoadSceneByName));
	global("void LoadScene(int sceneBuildIndex)", asFUNCTION(LoadSceneByIndex));
	global("Scene GetActiveScene()", asFUNCTION(GetActiveScene));
	global("int get_sceneCountInBuildSettings() property", asFUNCTION(GetSceneCountInBuildSettings));

	engine->SetDefaultNamespace("");
}
