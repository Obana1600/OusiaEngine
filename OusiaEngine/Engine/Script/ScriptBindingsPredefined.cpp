#include "ScriptBindings.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Platform/Paths.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#include <angelscript.h>

using namespace ou;


// AngelScript Language Server（VS Code の拡張機能）が読む as.predefined を書き出す
// 書き方は angel-lsp の examples/generate_as_predefined.cpp と同じ
namespace {

// get_x / set_x をプロパティとして登録したものは、AngelScript の書き方どおり property を付ける（補完で transform.position と出るように）
std::string DeclarationOf(const asIScriptFunction *function, bool includeObjectName, bool includeNamespace) {
	std::string declaration = function->GetDeclaration(includeObjectName, includeNamespace, true);
	if (function->IsProperty()) { declaration += " property"; }
	return declaration;
}

void WriteEnums(const asIScriptEngine *engine, std::ostream &stream) {
	for (asUINT i = 0; i < engine->GetEnumCount(); i++) {
		const auto *type = engine->GetEnumByIndex(i);
		if (!type) { continue; }

		const std::string_view ns = type->GetNamespace();
		if (!ns.empty()) { stream << std::format("namespace {} {{\n", ns); }
		stream << std::format("enum {} {{\n", type->GetName());
		for (asUINT j = 0; j < type->GetEnumValueCount(); j++) {
			asINT64 value = 0;
			const auto *name = type->GetEnumValueByIndex(j, &value);
			stream << std::format("\t{} = {}{}\n", name, value, j + 1 < type->GetEnumValueCount() ? "," : "");
		}
		stream << "}\n";
		if (!ns.empty()) { stream << "}\n"; }
	}
}

void WriteTypes(const asIScriptEngine *engine, std::ostream &stream) {
	for (asUINT i = 0; i < engine->GetObjectTypeCount(); i++) {
		const auto *type = engine->GetObjectTypeByIndex(i);
		if (!type) { continue; }

		const std::string_view ns = type->GetNamespace();
		if (!ns.empty()) { stream << std::format("namespace {} {{\n", ns); }

		stream << std::format("class {}", type->GetName());
		if (type->GetSubTypeCount() > 0) {
			stream << "<";
			for (asUINT sub = 0; sub < type->GetSubTypeCount(); sub++) {
				if (sub > 0) { stream << ", "; }
				stream << type->GetSubType(sub)->GetName();
			}
			stream << ">";
		}
		stream << " {\n";

		for (asUINT j = 0; j < type->GetBehaviourCount(); j++) {
			asEBehaviours behaviour{};
			const auto *function = type->GetBehaviourByIndex(j, &behaviour);
			if (behaviour == asBEHAVE_CONSTRUCT || behaviour == asBEHAVE_DESTRUCT) {
				stream << std::format("\t{};\n", function->GetDeclaration(false, true, true));
			}
		}
		for (asUINT j = 0; j < type->GetFactoryCount(); j++) {
			// 工場関数は、コンストラクタの形で書く（GameObject("name") と書けるように）
			const auto *factory = type->GetFactoryByIndex(j);
			std::string declaration = factory->GetDeclaration(false, true, true);
			if (const auto open = declaration.find('('); open != std::string::npos) {
				declaration = std::string(type->GetName()) + declaration.substr(open);
			}
			stream << std::format("\t{};\n", declaration);
		}
		for (asUINT j = 0; j < type->GetMethodCount(); j++) {
			stream << std::format("\t{};\n", DeclarationOf(type->GetMethodByIndex(j), false, true));
		}
		for (asUINT j = 0; j < type->GetPropertyCount(); j++) {
			stream << std::format("\t{};\n", type->GetPropertyDeclaration(j, true));
		}
		stream << "}\n";
		if (!ns.empty()) { stream << "}\n"; }
	}
}

void WriteGlobalFunctions(const asIScriptEngine *engine, std::ostream &stream) {
	for (asUINT i = 0; i < engine->GetGlobalFunctionCount(); i++) {
		const auto *function = engine->GetGlobalFunctionByIndex(i);
		if (!function) { continue; }

		const std::string_view ns = function->GetNamespace();
		if (!ns.empty()) { stream << std::format("namespace {} {{ ", ns); }
		stream << std::format("{};", DeclarationOf(function, false, false));
		if (!ns.empty()) { stream << " }"; }
		stream << "\n";
	}
}

void WriteGlobalProperties(const asIScriptEngine *engine, std::ostream &stream) {
	for (asUINT i = 0; i < engine->GetGlobalPropertyCount(); i++) {
		const char *name = nullptr;
		const char *ns = nullptr;
		int typeId = 0;
		bool isConst = false;
		engine->GetGlobalPropertyByIndex(i, &name, &ns, &typeId, &isConst);

		const std::string type = engine->GetTypeDeclaration(typeId, true);
		if (type.empty()) { continue; }

		const std::string_view nsView = ns ? ns : "";
		if (!nsView.empty()) { stream << std::format("namespace {} {{ ", nsView); }
		stream << std::format("{}{} {};", isConst ? "const " : "", type, name);
		if (!nsView.empty()) { stream << " }"; }
		stream << "\n";
	}
}

// スクリプト側の基底クラス（EngineAssets/Script/*.as）も、補完に出るように書き足す
void WriteEngineScripts(std::ostream &stream) {
	const auto folder = FindEngineAssetsFolder();
	if (!folder) { return; }

	std::error_code error;
	for (const auto &entry : std::filesystem::directory_iterator(*folder / L"Script", error)) {
		if (entry.path().extension() != L".as") { continue; }

		std::ifstream file(entry.path(), std::ios::binary);
		std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
		if (text.starts_with("\xEF\xBB\xBF")) { text.erase(0, 3); }
		stream << "\n// " << ToUtf8(entry.path().filename()) << "\n" << text << "\n";
	}
}

} // namespace



void ou::ScriptBindings::WritePredefined(asIScriptEngine *engine) {
#ifdef WITH_EDITOR
	// プロジェクトのフォルダ（Assets の親）の直下。VS Code でこのフォルダを開くと読まれる
	const auto assets = FindAssetsFolder();
	if (!assets) { return; }
	const auto path = assets->parent_path() / L"as.predefined";

	std::ostringstream stream;
	stream << "// エンジンが書き出したファイル。手で書き換えても、エディタを起動するたびに上書きされます\n";
	WriteEnums(engine, stream);
	WriteTypes(engine, stream);
	WriteGlobalFunctions(engine, stream);
	WriteGlobalProperties(engine, stream);
	WriteEngineScripts(stream);

	std::ofstream file(path, std::ios::binary);
	if (!file) {
		Log::Warning("as.predefined を書き出せませんでした: {}", ToUtf8(path));
		return;
	}
	file << stream.str();
#else
	(void)engine;
#endif
}
