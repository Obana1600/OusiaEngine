#include "LogEntries.h"
#include "Editor/EditorPrefs.h"

#include <chrono>
#include <format>
#include <string>
#include <utility>

using namespace ou;


namespace {

constexpr const char *kClearOnPlayKey = "Console.ClearOnPlay";

// 取りに来られないまま溜まりすぎないように（コンソールの更新が止まっていても、メモリを食い尽くさない）
constexpr size_t kMaxPending = 10000;

std::string MakeTimeText(std::chrono::system_clock::time_point time) {
	const auto local = std::chrono::current_zone()->to_local(std::chrono::floor<std::chrono::seconds>(time));
	return std::format("{:%H:%M:%S}", local);
}

} // namespace



void ou::LogEntries::Initialize() {
	SetLogListener(&LogEntries::OnLogMessage);
}


void ou::LogEntries::Finalize() {
	SetLogListener(nullptr);

	std::scoped_lock lock(sMutex_);
	sPending_.clear();
	sEntries_.clear();
}


void ou::LogEntries::Update() {
	std::vector<Entry> pending;
	{
		std::scoped_lock lock(sMutex_);
		pending.swap(sPending_);
	}
	if (pending.empty()) { return; }

	for (auto &entry : pending) {
		entry.serial = sNextSerial_++;
		entry.timeText = MakeTimeText(entry.time);
		sCounts_[static_cast<size_t>(entry.level)]++;
		sEntries_.push_back(std::move(entry));
	}

	while (sEntries_.size() > kMaxEntries) {
		sCounts_[static_cast<size_t>(sEntries_.front().level)]--;
		sEntries_.pop_front();
	}

	sVersion_++;
}


void ou::LogEntries::Clear() {
	{
		std::scoped_lock lock(sMutex_);
		sPending_.clear();
	}
	sEntries_.clear();
	sCounts_ = {};
	sVersion_++;
}


void ou::LogEntries::ClearOnPlayIfEnabled() {
	if (IsClearOnPlay()) {
		Clear();
	}
}


bool ou::LogEntries::IsClearOnPlay() {
	return EditorPrefs::GetBool(kClearOnPlayKey, true);
}


void ou::LogEntries::SetClearOnPlay(bool enabled) {
	EditorPrefs::SetBool(kClearOnPlayKey, enabled);
}


const std::deque<LogEntries::Entry> &ou::LogEntries::GetEntries() {
	return sEntries_;
}


uint64_t ou::LogEntries::GetVersion() {
	return sVersion_;
}


int ou::LogEntries::GetCount(LogLevel level) {
	return sCounts_[static_cast<size_t>(level)];
}


void ou::LogEntries::OnLogMessage(const LogMessage &message) {
	Entry entry{
		.level = message.level,
		.message = std::string(message.message),
		.file = std::string(message.file),
		.line = message.line,
		.isScript = message.isScript,
		.time = std::chrono::system_clock::now(),
	};
	entry.firstLineLength = entry.message.find('\n');
	if (entry.firstLineLength == std::string::npos) {
		entry.firstLineLength = entry.message.size();
	}

	std::scoped_lock lock(sMutex_);
	if (sPending_.size() >= kMaxPending) {
		sPending_.erase(sPending_.begin());
	}
	sPending_.push_back(std::move(entry));
}
