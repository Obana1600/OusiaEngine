#include "ScriptEngine.h"
#include "Asset/AssetDatabase.h"
#include "Core/JsonFile.h"
#include "Diagnostics/Log.h"
#include "Physics/Collider.h"
#include "Physics/Collision.h"
#include "Platform/Paths.h"
#include "Script/MonoBehavior.h"
#include "Script/ScriptBindings.h"
#include "Serialize/Serializer.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string_view>
#include <system_error>

#include <angelscript.h>
#include <scriptarray/scriptarray.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptstdstring/scriptstdstring.h>

using namespace ou;


namespace {

constexpr const char *kModuleName = "Assembly-AngelScript";
constexpr const char *kBaseClassName = "MonoBehavior";
constexpr std::string_view kNativePropertyName = "native_";

std::optional<std::string> ReadText(const std::filesystem::path &path) {
	std::ifstream file(path, std::ios::binary);
	if (!file) { return std::nullopt; }

	std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

	if (text.starts_with("\xEF\xBB\xBF")) { text.erase(0, 3); }
	return text;
}

} // namespace



void ou::ScriptEngine::Initialize() {
	sEngine_ = asCreateScriptEngine();
	if (!sEngine_) {
		Log::Error("AngelScript のエンジンの作成に失敗しました");
		return;
	}

	// コンパイルのエラーと警告をログに流す
	sEngine_->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL);

	// アドオン
	RegisterStdString(sEngine_);
	RegisterScriptArray(sEngine_, true);

	ScriptBindings::Register(sEngine_);

	ScriptBindings::WritePredefined(sEngine_);

	Compile();
}


void ou::ScriptEngine::Finalize() {
	if (!sEngine_) { return; }

	sModule_ = nullptr;
	sEngine_->ShutDownAndRelease();
	sEngine_ = nullptr;
}


bool ou::ScriptEngine::Compile() {
	if (!sEngine_) { return false; }

	sModule_ = nullptr;
	sScriptTimes_ = FindScriptFiles();

	CScriptBuilder builder;
	if (builder.StartNewModule(sEngine_, kModuleName) < 0) {
		Log::Error("スクリプトのモジュールの作成に失敗しました");
		return false;
	}

	const auto guids = AssetDatabase::FindAssets("t:MonoScript");
	for (const auto &guid : guids) {
		const auto path = AssetDatabase::GuidToAssetPath(guid);
		const auto fullPath = AssetDatabase::AssetPathToFullPath(path);
		const auto code = fullPath ? ReadText(*fullPath) : std::nullopt;
		if (!code) {
			Log::Error("スクリプトの読み込みに失敗しました: {}", path);
			continue;
		}

		builder.AddSectionFromMemory(path.c_str(), code->c_str(), static_cast<unsigned int>(code->size()));
	}

	if (builder.BuildModule() < 0) {
		Log::Error("スクリプトのコンパイルに失敗しました");
		return false;
	}
	sModule_ = sEngine_->GetModule(kModuleName);

	sSerializeFields_.clear();
	for (asUINT t = 0; t < sModule_->GetObjectTypeCount(); t++) {
		const auto *type = sModule_->GetObjectTypeByIndex(t);
		for (asUINT i = 0; i < type->GetPropertyCount(); i++) {
			for (const auto &metadata : builder.GetMetadataForTypeProperty(type->GetTypeId(), static_cast<int>(i))) {
				if (metadata == "SerializeField") { sSerializeFields_.insert({type->GetTypeId(), i}); }
			}
		}
	}

	Log::Info("スクリプトをコンパイルしました ({} ファイル)", guids.size());
	return true;
}


bool ou::ScriptEngine::IsOutdated() {
	return FindScriptFiles() != sScriptTimes_;
}


std::map<std::filesystem::path, std::filesystem::file_time_type> ou::ScriptEngine::FindScriptFiles() {
	std::map<std::filesystem::path, std::filesystem::file_time_type> files;

	for (const auto &folder : {FindAssetsFolder(), FindEngineAssetsFolder()}) {
		if (!folder) { continue; }

		std::error_code error;
		for (std::filesystem::recursive_directory_iterator it(*folder, std::filesystem::directory_options::skip_permission_denied, error), end;
			 it != end;
			 it.increment(error)) {
			if (error) { break; }
			if (it->is_regular_file(error) && it->path().extension() == L".as") {
				files[it->path()] = it->last_write_time(error);
			}
		}
	}

	return files;
}


asITypeInfo *ou::ScriptEngine::FindMonoBehaviorClass(const std::string &className) {
	if (!sModule_) {
		Log::Warning("スクリプトがコンパイルされていないため、{} を作れません", className);
		return nullptr;
	}

	auto *type = sModule_->GetTypeInfoByName(className.c_str());
	auto *baseType = sModule_->GetTypeInfoByName(kBaseClassName);
	if (!type || !baseType || type == baseType || !type->DerivesFrom(baseType)) {
		Log::Warning("MonoBehavior を継承したクラス {} が見つかりません。ファイル名とクラス名を一致させてください", className);
		return nullptr;
	}

	return type;
}


asIScriptObject *ou::ScriptEngine::CreateObject(asITypeInfo *type, MonoBehavior *native) {
	auto *object = static_cast<asIScriptObject *>(sEngine_->CreateScriptObject(type));
	if (!object) {
		Log::Error("スクリプトのオブジェクトの作成に失敗しました: {}", type->GetName());
		return nullptr;
	}

	for (asUINT i = 0; i < object->GetPropertyCount(); i++) {
		if (object->GetPropertyName(i) == kNativePropertyName) {
			*static_cast<void **>(object->GetAddressOfProperty(i)) = ScriptBindings::CreateObjectHandle(native);
			break;
		}
	}

	return object;
}


bool ou::ScriptEngine::CallMethod(asIScriptObject *object, asIScriptFunction *method, void *argument) {
	if (!sEngine_ || !object || !method) { return false; }

	auto *context = sEngine_->RequestContext();
	context->Prepare(method);
	context->SetObject(object);

	if (argument) { context->SetArgObject(0, argument); }

	const auto result = context->Execute();
	if (result == asEXECUTION_EXCEPTION) {
		LogException(context);
	}

	sEngine_->ReturnContext(context);
	return result == asEXECUTION_FINISHED;
}


void ou::ScriptEngine::CallCollisionMethod(asIScriptObject *object, asIScriptFunction *method, const Collision &collision) {
	auto *argument = ScriptBindings::CreateCollision(collision);
	CallMethod(object, method, argument);
	ScriptBindings::DestroyCollision(argument);
}


void ou::ScriptEngine::CallColliderMethod(asIScriptObject *object, asIScriptFunction *method, Collider *collider) {
	auto *handle = ScriptBindings::CreateObjectHandle(collider);
	CallMethod(object, method, handle);
	ScriptBindings::ReleaseObjectHandle(handle);
}


void ou::ScriptEngine::LogException(asIScriptContext *context) {
	const char *section = nullptr;
	const auto row = context->GetExceptionLineNumber(nullptr, &section);
	const auto *function = context->GetExceptionFunction();
	const auto *message = context->GetExceptionString();

	Log::Error("{}\n({}:{} {})",
			   message ? message : "?",
			   section ? section : "?",
			   row,
			   function ? function->GetDeclaration() : "?");
}


void ou::ScriptEngine::SerializeFields(asIScriptObject *object, Serializer &serializer) {
	const auto *type = object->GetObjectType();
	for (asUINT i = 0; i < type->GetPropertyCount(); i++) {
		if (!IsSerializedField(type, i)) { continue; }

		const char *name = nullptr;
		int typeId = 0;
		type->GetProperty(i, &name, &typeId);
		ScriptBindings::SerializeProperty(serializer, name, typeId, object->GetAddressOfProperty(i));
	}
}


bool ou::ScriptEngine::IsSerializedField(const asITypeInfo *type, asUINT index) {
	// public な変数か調べる
	bool isPrivate = false;
	bool isProtected = false;
	type->GetProperty(index, nullptr, nullptr, &isPrivate, &isProtected);
	if (!isPrivate && !isProtected) { return true; }

	// 継承した変数か調べる
	for (const auto *current = type; current && index < current->GetPropertyCount(); current = current->GetBaseType()) {
		if (sSerializeFields_.contains({current->GetTypeId(), index})) { return true; }
	}

	return false;
}


void ou::ScriptEngine::MessageCallback(const asSMessageInfo *message, void *) {
	switch (message->type) {
		case asMSGTYPE_ERROR:
			Log::Error("{}({},{}): {}", message->section, message->row, message->col, message->message);
			break;
		case asMSGTYPE_WARNING:
			Log::Warning("{}({},{}): {}", message->section, message->row, message->col, message->message);
			break;
		default:
			Log::Info("{}({},{}): {}", message->section, message->row, message->col, message->message);
			break;
	}
}
