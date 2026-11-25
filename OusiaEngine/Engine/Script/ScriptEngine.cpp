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

#include <algorithm>
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

std::string RemoveSpaces(std::string_view text) {
	std::string result;
	for (const auto c : text) {
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n') { result += c; }
	}
	return result;
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
	sRuntimeInitializeMethods_.clear();
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

	CollectRuntimeInitializeMethods(builder);

	Log::Info("スクリプトをコンパイルしました ({} ファイル)", guids.size());
	return true;
}


void ou::ScriptEngine::ResetGlobalVars() {
	if (!sModule_) { return; }

	if (sModule_->ResetGlobalVars() < 0) {
		Log::Error("スクリプトのグローバル変数を初期値に戻せませんでした");
	}
}


void ou::ScriptEngine::InvokeBeforeSceneLoadMethods() {
	for (const auto loadType : {RuntimeInitializeLoadType::SubsystemRegistration,
								RuntimeInitializeLoadType::AfterAssembliesLoaded,
								RuntimeInitializeLoadType::BeforeSplashScreen,
								RuntimeInitializeLoadType::BeforeSceneLoad}) {
		InvokeRuntimeInitializeMethods(loadType);
	}
}


void ou::ScriptEngine::InvokeAfterSceneLoadMethods() {
	InvokeRuntimeInitializeMethods(RuntimeInitializeLoadType::AfterSceneLoad);
}


void ou::ScriptEngine::InvokeRuntimeInitializeMethods(RuntimeInitializeLoadType loadType) {
	for (const auto &method : sRuntimeInitializeMethods_) {
		if (method.loadType != loadType) { continue; }
		CallMethodWith(nullptr, method.function, [](asIScriptContext *) {});
	}
}


void ou::ScriptEngine::CollectRuntimeInitializeMethods(CScriptBuilder &builder) {
	constexpr std::string_view kAttribute = "RuntimeInitializeOnLoadMethod";
	constexpr std::string_view kEnumPrefix = "RuntimeInitializeLoadType.";
	constexpr std::pair<std::string_view, RuntimeInitializeLoadType> kLoadTypes[] = {
		{"SubsystemRegistration", RuntimeInitializeLoadType::SubsystemRegistration},
		{"AfterAssembliesLoaded", RuntimeInitializeLoadType::AfterAssembliesLoaded},
		{"BeforeSplashScreen", RuntimeInitializeLoadType::BeforeSplashScreen},
		{"BeforeSceneLoad", RuntimeInitializeLoadType::BeforeSceneLoad},
		{"AfterSceneLoad", RuntimeInitializeLoadType::AfterSceneLoad},
	};

	for (asUINT i = 0; i < sModule_->GetFunctionCount(); i++) {
		auto *function = sModule_->GetFunctionByIndex(i);

		for (const auto &metadata : builder.GetMetadataForFunc(function)) {
			const auto text = RemoveSpaces(metadata);
			if (!text.starts_with(kAttribute)) { continue; }

			// 引数がなければ、Unity と同じく AfterSceneLoad
			std::string_view argument = std::string_view(text).substr(kAttribute.size());
			if (argument.empty() || argument == "()") {
				argument = "AfterSceneLoad";
			} else if (argument.starts_with('(') && argument.ends_with(')')) {
				argument = argument.substr(1, argument.size() - 2);
				if (argument.starts_with(kEnumPrefix)) { argument.remove_prefix(kEnumPrefix.size()); }
			} else {
				continue; // RuntimeInitializeOnLoadMethodXxx のような別の名前
			}

			const char *section = nullptr;
			int row = 0;
			function->GetDeclaredAt(&section, &row, nullptr);
			const auto *found = std::find_if(std::begin(kLoadTypes), std::end(kLoadTypes), [&](const auto &pair) { return pair.first == argument; });
			if (found == std::end(kLoadTypes)) {
				Log::Warning("{}({}): RuntimeInitializeLoadType に {} はありません", section ? section : "", row, argument);
				continue;
			}
			if (function->GetParamCount() != 0 || function->GetReturnTypeId() != asTYPEID_VOID) {
				Log::Warning("{}({}): [RuntimeInitializeOnLoadMethod] は引数のない void の関数に付けてください: {}",
							 section ? section : "", row, function->GetDeclaration(true, true));
				continue;
			}

			sRuntimeInitializeMethods_.push_back({found->second, function});
		}
	}
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
	return CallMethodWith(object, method, [argument](asIScriptContext *context) {
		if (argument) { context->SetArgObject(0, argument); }
	});
}


bool ou::ScriptEngine::CallMethodWith(asIScriptObject *object, asIScriptFunction *method, const std::function<void(asIScriptContext *)> &setArguments) {
	if (!sEngine_ || !method) { return false; }

	auto *context = sEngine_->RequestContext();
	context->Prepare(method);
	if (object) { context->SetObject(object); } // null ならメソッドではない関数（delegate など）
	setArguments(context);

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


void ou::ScriptEngine::CallControllerColliderHitMethod(asIScriptObject *object, asIScriptFunction *method, const ControllerColliderHit &hit) {
	auto *argument = ScriptBindings::CreateControllerColliderHit(hit);
	CallMethod(object, method, argument);
	ScriptBindings::DestroyControllerColliderHit(argument);
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

	Log::Write(LogLevel::Error, std::source_location::current(),
			   std::format("{}\n{}", message ? message : "?", function ? function->GetDeclaration() : "?"),
			   {section ? section : "", row});
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
	auto level = LogLevel::Info;
	if (message->type == asMSGTYPE_ERROR) {
		level = LogLevel::Error;
	} else if (message->type == asMSGTYPE_WARNING) {
		level = LogLevel::Warning;
	}

	// ファイルに残すログでも分かるように、場所は本文にも残す
	const auto *section = message->section ? message->section : "";
	Log::Write(level, std::source_location::current(),
			   std::format("{}({},{}): {}", section, message->row, message->col, message->message),
			   {section, message->row});
}
