#include "MonoScriptImporter.h"
#include "Core/StringUtil.h"
#include "Script/MonoScript.h"

#include <utility>

using namespace ou;


ou::MonoScriptImporter::MonoScriptImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::MonoScriptImporter::Import() {
	auto script = std::make_shared<MonoScript>();
	script->SetName(ToUtf8(GetFullPath().stem()));
	return script;
}
