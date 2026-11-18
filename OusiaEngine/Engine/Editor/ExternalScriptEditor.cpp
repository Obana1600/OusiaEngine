#include "ExternalScriptEditor.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorPrefs.h"
#include "Platform/Paths.h"

#include <cstdio>
#include <cwctype>
#include <format>
#include <string>
#include <system_error>

#include <Windows.h>

using namespace ou;


namespace {

// Unity の EditorPrefs のキーと同じ名前
constexpr const char *kEditorKey = "kScriptsDefaultApp";

std::wstring EnvironmentVariable(const wchar_t *name) {
	wchar_t buffer[MAX_PATH] = {};
	const auto length = GetEnvironmentVariableW(name, buffer, MAX_PATH);
	return length > 0 && length < MAX_PATH ? std::wstring(buffer, length) : std::wstring();
}

bool Exists(const std::filesystem::path &path) {
	std::error_code error;
	return !path.empty() && std::filesystem::is_regular_file(path, error);
}

std::wstring LowerFileName(const std::filesystem::path &path) {
	auto name = path.filename().wstring();
	for (auto &c : name) { c = static_cast<wchar_t>(std::towlower(c)); }
	return name;
}

// VS Code は、ユーザーごと・PC 全体のどちらにも入る
std::filesystem::path FindVisualStudioCode() {
	const std::filesystem::path candidates[] = {
		std::filesystem::path(EnvironmentVariable(L"LOCALAPPDATA")) / L"Programs/Microsoft VS Code/Code.exe",
		std::filesystem::path(EnvironmentVariable(L"ProgramFiles")) / L"Microsoft VS Code/Code.exe",
	};
	for (const auto &candidate : candidates) {
		if (Exists(candidate)) { return candidate; }
	}
	return {};
}

// Visual Studio は、インストーラーの vswhere に聞く
std::filesystem::path FindVisualStudio() {
	const auto vswhere = std::filesystem::path(EnvironmentVariable(L"ProgramFiles(x86)")) / L"Microsoft Visual Studio/Installer/vswhere.exe";
	if (!Exists(vswhere)) { return {}; }

	const auto command = std::format(L"\"\"{}\" -latest -property productPath\"", vswhere.wstring());
	auto *pipe = _wpopen(command.c_str(), L"rt");
	if (!pipe) { return {}; }

	char buffer[1024] = {};
	std::string output;
	while (std::fgets(buffer, sizeof(buffer), pipe)) { output += buffer; }
	_pclose(pipe);

	while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) { output.pop_back(); }
	const auto path = std::filesystem::path(Utf8ToWide(output));
	return Exists(path) ? path : std::filesystem::path();
}

// プロジェクトのフォルダ（Assets の親）。VS Code はこれを開くと、as.predefined を読んで補完が効く
std::filesystem::path ProjectFolder() {
	const auto assets = FindAssetsFolder();
	return assets ? assets->parent_path() : std::filesystem::path();
}

std::wstring MakeCommandLine(const std::filesystem::path &editor, const std::filesystem::path &file, int line) {
	const auto name = LowerFileName(editor);
	if (name == L"code.exe") {
		const auto location = line > 0 ? std::format(L"{}:{}", file.wstring(), line) : file.wstring();
		return std::format(L"\"{}\" \"{}\" -g \"{}\"", editor.wstring(), ProjectFolder().wstring(), location);
	}
	if (name == L"devenv.exe") {
		return std::format(L"\"{}\" /Edit \"{}\"", editor.wstring(), file.wstring());
	}
	return std::format(L"\"{}\" \"{}\"", editor.wstring(), file.wstring());
}

} // namespace



std::vector<ExternalScriptEditor::Installation> ou::ExternalScriptEditor::FindInstallations() {
	std::vector<Installation> installations;
	if (auto path = FindVisualStudioCode(); !path.empty()) { installations.push_back({"Visual Studio Code", path}); }
	if (auto path = FindVisualStudio(); !path.empty()) { installations.push_back({"Visual Studio", path}); }

	const auto notepad = std::filesystem::path(EnvironmentVariable(L"SystemRoot")) / L"System32/notepad.exe";
	if (Exists(notepad)) { installations.push_back({"メモ帳", notepad}); }
	return installations;
}


std::filesystem::path ou::ExternalScriptEditor::FindCurrentPath() {
	const auto saved = std::filesystem::path(Utf8ToWide(EditorPrefs::GetString(kEditorKey)));
	if (Exists(saved)) { return saved; }

	// 選んでいなければ VS Code、なければメモ帳（毎フレーム呼ばれるので、時間のかかる Visual Studio は探さない）
	if (auto code = FindVisualStudioCode(); !code.empty()) { return code; }
	const auto notepad = std::filesystem::path(EnvironmentVariable(L"SystemRoot")) / L"System32/notepad.exe";
	return Exists(notepad) ? notepad : std::filesystem::path();
}


void ou::ExternalScriptEditor::SetCurrentPath(const std::filesystem::path &path) {
	EditorPrefs::SetString(kEditorKey, ToUtf8(path));
}


bool ou::ExternalScriptEditor::OpenFile(const std::filesystem::path &file, int line) {
	const auto editor = FindCurrentPath();
	if (editor.empty()) {
		Log::Error("スクリプトを開くエディタが見つかりません。Edit > Preferences で選んでください");
		return false;
	}

	auto commandLine = MakeCommandLine(editor, file, line);
	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);
	PROCESS_INFORMATION process{};
	if (!CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process)) {
		Log::Error("エディタを起動できませんでした ({}): {}", GetLastError(), ToUtf8(commandLine));
		return false;
	}

	// 待たない（エディタはエディタで動き続ける）
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return true;
}
