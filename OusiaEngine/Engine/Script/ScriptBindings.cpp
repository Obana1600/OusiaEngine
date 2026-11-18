#include "ScriptBindings.h"
#include "Core/Application.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Input/Input.h"
#include "Input/KeyCode.h"
#include "Math/Mathf.h"
#include "Math/Random.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Platform/Screen.h"
#include "Script/ScriptBindingsInternal.h"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <string>

#include <angelscript.h>
#include <magic_enum/magic_enum.hpp>


// KeyCode は 0xA5 まであり、magic_enum が既定で見る範囲（127 まで）を超えるので広げる
// KeyCode を magic_enum に渡すのはこの .cpp だけ
template <>
struct magic_enum::customize::enum_range<ou::KeyCode> {
	static constexpr int min = 0;
	static constexpr int max = 255;
};


using namespace ou;
using ScriptBindingsInternal::Check;
using ScriptBindingsInternal::ThrowException;


namespace {

// ---- Debug ----

// 呼び出したスクリプトの場所（Unity のコンソールのスタックトレースの代わり）
std::string DescribeScriptLocation() {
	auto *context = asGetActiveContext();
	if (!context) { return {}; }

	const char *section = nullptr;
	const auto row = context->GetLineNumber(0, nullptr, &section);
	return std::format("\n({}:{})", section ? section : "?", row);
}

void DebugLog(const std::string &message) {
	Log::Info("{}{}", message, DescribeScriptLocation());
}

void DebugLogWarning(const std::string &message) {
	Log::Warning("{}{}", message, DescribeScriptLocation());
}

void DebugLogError(const std::string &message) {
	Log::Error("{}{}", message, DescribeScriptLocation());
}


// ---- Mathf ----

// AngelScript の const なグローバル変数はアドレスで登録するので、置き場所を作る
const float kPi = Mathf::PI;
const float kDeg2Rad = Mathf::Deg2Rad;
const float kRad2Deg = Mathf::Rad2Deg;
const float kInfinity = Mathf::Infinity;
const float kNegativeInfinity = Mathf::NegativeInfinity;
const float kEpsilon = Mathf::Epsilon;

float MoveTowards(float current, float target, float maxDelta) {
	if (Mathf::Abs(target - current) <= maxDelta) { return target; }
	return current + Mathf::Sign(target - current) * maxDelta;
}

float PingPong(float t, float length) {
	const auto value = Mathf::Repeat(t, length * 2.0f);
	return length - Mathf::Abs(value - length);
}

int AbsInt(int value) { return std::abs(value); }
int MinInt(int a, int b) { return std::min(a, b); }
int MaxInt(int a, int b) { return std::max(a, b); }
int ClampInt(int value, int min, int max) { return std::clamp(value, min, std::max(min, max)); }


// ---- Random ----

void InitRandomState(int seed) {
	Random::InitState(static_cast<uint32_t>(seed));
}


// ---- Screen ----
// スクリプトの enum と int は int で受け渡す
int GetScreenWidth() { return static_cast<int>(Screen::GetWidth()); }
int GetScreenHeight() { return static_cast<int>(Screen::GetHeight()); }
int GetFullScreenMode() { return static_cast<int>(Screen::GetFullScreenMode()); }
void SetFullScreenMode(int mode) {
	if (magic_enum::enum_contains<FullScreenMode>(mode)) { Screen::SetFullScreenMode(static_cast<FullScreenMode>(mode)); }
}
void SetResolution(int width, int height, int mode) {
	if (width <= 0 || height <= 0 || !magic_enum::enum_contains<FullScreenMode>(mode)) { return; }
	Screen::SetResolution(static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<FullScreenMode>(mode));
}

// ---- Time ----

int GetFrameCount() {
	return static_cast<int>(Time::GetFrameCount());
}


// ---- Input ----

// AngelScript の enum は 32 ビットなので int で受ける
bool GetKey(int key) { return Input::GetKey(static_cast<KeyCode>(key)); }
bool GetKeyDown(int key) { return Input::GetKeyDown(static_cast<KeyCode>(key)); }
bool GetKeyUp(int key) { return Input::GetKeyUp(static_cast<KeyCode>(key)); }

// Unity と同じく Vector3 で返す（z は 0）。原点はエンジンと同じく Game ビューの左上
Vector3 GetMousePosition() {
	const auto position = Input::GetMousePosition();
	return {position.x, position.y, 0.0f};
}

Vector2 GetMouseScrollDelta() {
	return Input::GetMouseScrollDelta();
}

float KeyAxis(KeyCode positive, KeyCode alternatePositive, KeyCode negative, KeyCode alternateNegative) {
	auto value = 0.0f;
	if (Input::GetKey(positive) || Input::GetKey(alternatePositive)) { value += 1.0f; }
	if (Input::GetKey(negative) || Input::GetKey(alternateNegative)) { value -= 1.0f; }
	return value;
}

// Unity の Input Manager の既定の軸を決め打ちにする
// なめらかにする処理はないので、GetAxis と GetAxisRaw は同じ値を返す
float GetAxis(const std::string &axisName) {
	if (axisName == "Horizontal") {
		const auto value = KeyAxis(KeyCode::D, KeyCode::RightArrow, KeyCode::A, KeyCode::LeftArrow) + Input::GetGamepadLeftStick().x;
		return Mathf::Clamp(value, -1.0f, 1.0f);
	}
	if (axisName == "Vertical") {
		const auto value = KeyAxis(KeyCode::W, KeyCode::UpArrow, KeyCode::S, KeyCode::DownArrow) + Input::GetGamepadLeftStick().y;
		return Mathf::Clamp(value, -1.0f, 1.0f);
	}
	// Unity の既定の感度 0.1 に合わせる。画面の座標は下が + なので、Mouse Y は向きを反対にする
	if (axisName == "Mouse X") { return Input::GetMouseDelta().x * 0.1f; }
	if (axisName == "Mouse Y") { return -Input::GetMouseDelta().y * 0.1f; }
	if (axisName == "Mouse ScrollWheel") { return Input::GetMouseScrollDelta().y * 0.1f; }

	ThrowException(std::format("ArgumentException: Input Axis {} is not setup. (使える軸: Horizontal, Vertical, Mouse X, Mouse Y, Mouse ScrollWheel)", axisName));
	return 0.0f;
}

} // namespace



void ou::ScriptBindings::Register(asIScriptEngine *engine) {
	// 型を先に登録する（ほかの型の関数の宣言に出てくるので）
	RegisterValueTypes(engine);
	RegisterObjectTypes(engine);
	RegisterPhysicsTypes(engine);
	RegisterSceneTypes(engine);
	RegisterObjectMembers(engine);
	RegisterPhysicsMembers(engine);
	RegisterComponentMembers(engine);
	RegisterGetComponent(engine);
	RegisterSceneManager(engine);

	RegisterDebug(engine);
	RegisterMathf(engine);
	RegisterRandom(engine);
	RegisterTime(engine);
	RegisterInput(engine);
	RegisterScreen(engine);
	RegisterApplication(engine);
}


void ou::ScriptBindings::RegisterRandom(asIScriptEngine *engine) {
	// Unity と同じく、int の Range は max を含まない。float の Range は max を含む
	engine->SetDefaultNamespace("Random");

	const auto global = [&](const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterGlobalFunction(declaration, function, asCALL_CDECL), declaration);
	};
	global("void InitState(int seed)", asFUNCTION(InitRandomState));
	global("float get_value() property", asFUNCTION(Random::Value));
	global("float Range(float minInclusive, float maxInclusive)", asFUNCTIONPR(Random::Range, (float, float), float));
	global("int Range(int minInclusive, int maxExclusive)", asFUNCTIONPR(Random::Range, (int, int), int));
	global("Vector3 get_insideUnitSphere() property", asFUNCTION(Random::InsideUnitSphere));
	global("Vector3 get_onUnitSphere() property", asFUNCTION(Random::OnUnitSphere));
	global("Vector2 get_insideUnitCircle() property", asFUNCTION(Random::InsideUnitCircle));

	engine->SetDefaultNamespace("");
}


void ou::ScriptBindings::RegisterApplication(asIScriptEngine *engine) {
	// Unity と同じく、エディタでは Quit しても何も起きない（Play を止めるのは EditorApplication.isPlaying）
	engine->SetDefaultNamespace("Application");

	const auto global = [&](const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterGlobalFunction(declaration, function, asCALL_CDECL), declaration);
	};
	global("void Quit()", asFUNCTION(Application::Quit));
	global("bool get_isPlaying() property", asFUNCTION(Application::IsPlaying));

	engine->SetDefaultNamespace("");
}


void ou::ScriptBindings::RegisterDebug(asIScriptEngine *engine) {
	// Unity の Debug.Log(object) の代わりに string を受け取る（数は "x = " + x で文字列にできる）
	engine->SetDefaultNamespace("Debug");

	const auto global = [&](const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterGlobalFunction(declaration, function, asCALL_CDECL), declaration);
	};
	global("void Log(const string &in message)", asFUNCTION(DebugLog));
	global("void LogWarning(const string &in message)", asFUNCTION(DebugLogWarning));
	global("void LogError(const string &in message)", asFUNCTION(DebugLogError));

	engine->SetDefaultNamespace("");
}


void ou::ScriptBindings::RegisterMathf(asIScriptEngine *engine) {
	engine->SetDefaultNamespace("Mathf");

	const auto constant = [&](const char *declaration, const float &value) {
		Check(engine->RegisterGlobalProperty(declaration, const_cast<float *>(&value)), declaration);
	};
	constant("const float PI", kPi);
	constant("const float Deg2Rad", kDeg2Rad);
	constant("const float Rad2Deg", kRad2Deg);
	constant("const float Infinity", kInfinity);
	constant("const float NegativeInfinity", kNegativeInfinity);
	constant("const float Epsilon", kEpsilon);

	const auto global = [&](const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterGlobalFunction(declaration, function, asCALL_CDECL), declaration);
	};
	global("float Abs(float f)", asFUNCTIONPR(Mathf::Abs, (float), float));
	global("int Abs(int value)", asFUNCTION(AbsInt));
	global("float Acos(float f)", asFUNCTION(Mathf::Acos));
	global("float Asin(float f)", asFUNCTION(Mathf::Asin));
	global("float Atan2(float y, float x)", asFUNCTION(Mathf::Atan2));
	global("bool Approximately(float a, float b)", asFUNCTION(Mathf::Approximately));
	global("float Ceil(float f)", asFUNCTION(Mathf::Ceil));
	global("int CeilToInt(float f)", asFUNCTION(Mathf::CeilToInt));
	global("float Clamp(float value, float min, float max)", asFUNCTION(Mathf::Clamp));
	global("int Clamp(int value, int min, int max)", asFUNCTION(ClampInt));
	global("float Clamp01(float value)", asFUNCTION(Mathf::Clamp01));
	global("float Cos(float f)", asFUNCTION(Mathf::Cos));
	global("float Exp(float power)", asFUNCTION(Mathf::Exp));
	global("float Floor(float f)", asFUNCTION(Mathf::Floor));
	global("int FloorToInt(float f)", asFUNCTION(Mathf::FloorToInt));
	global("float InverseLerp(float a, float b, float value)", asFUNCTION(Mathf::InverseLerp));
	global("float Lerp(float a, float b, float t)", asFUNCTION(Mathf::Lerp));
	global("float LerpUnclamped(float a, float b, float t)", asFUNCTION(Mathf::LerpUnclamped));
	global("float Log(float f)", asFUNCTION(Mathf::Log));
	global("float Max(float a, float b)", asFUNCTIONPR(Mathf::Max, (float, float), float));
	global("int Max(int a, int b)", asFUNCTION(MaxInt));
	global("float Min(float a, float b)", asFUNCTIONPR(Mathf::Min, (float, float), float));
	global("int Min(int a, int b)", asFUNCTION(MinInt));
	global("float MoveTowards(float current, float target, float maxDelta)", asFUNCTION(MoveTowards));
	global("float PingPong(float t, float length)", asFUNCTION(PingPong));
	global("float Pow(float f, float p)", asFUNCTION(Mathf::Pow));
	global("float Repeat(float t, float length)", asFUNCTION(Mathf::Repeat));
	global("float Round(float f)", asFUNCTION(Mathf::Round));
	global("int RoundToInt(float f)", asFUNCTION(Mathf::RoundToInt));
	global("float Sign(float f)", asFUNCTION(Mathf::Sign));
	global("float Sin(float f)", asFUNCTION(Mathf::Sin));
	global("float Sqrt(float f)", asFUNCTION(Mathf::Sqrt));
	global("float Tan(float f)", asFUNCTION(Mathf::Tan));

	engine->SetDefaultNamespace("");
}


void ou::ScriptBindings::RegisterScreen(asIScriptEngine *engine) {
	// FullScreenMode::FullScreenWindow のように書く
	Check(engine->RegisterEnum("FullScreenMode"), "FullScreenMode");
	for (const auto mode : magic_enum::enum_values<FullScreenMode>()) {
		const std::string name(magic_enum::enum_name(mode));
		Check(engine->RegisterEnumValue("FullScreenMode", name.c_str(), static_cast<int>(mode)), name);
	}

	// Unity と同じくプロパティにする（Screen::width・Screen::fullScreen）
	engine->SetDefaultNamespace("Screen");

	const auto global = [&](const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterGlobalFunction(declaration, function, asCALL_CDECL), declaration);
	};
	global("int get_width() property", asFUNCTION(GetScreenWidth));
	global("int get_height() property", asFUNCTION(GetScreenHeight));
	global("FullScreenMode get_fullScreenMode() property", asFUNCTION(GetFullScreenMode));
	global("void set_fullScreenMode(FullScreenMode value) property", asFUNCTION(SetFullScreenMode));
	global("bool get_fullScreen() property", asFUNCTION(Screen::IsFullScreen));
	global("void set_fullScreen(bool value) property", asFUNCTION(Screen::SetFullScreen));
	global("void SetResolution(int width, int height, FullScreenMode mode)", asFUNCTION(SetResolution));

	engine->SetDefaultNamespace("");
}


void ou::ScriptBindings::RegisterTime(asIScriptEngine *engine) {
	// Unity と同じくプロパティにする（Time::deltaTime）
	engine->SetDefaultNamespace("Time");

	const auto global = [&](const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterGlobalFunction(declaration, function, asCALL_CDECL), declaration);
	};
	global("float get_deltaTime() property", asFUNCTION(Time::GetDeltaTime));
	global("float get_unscaledDeltaTime() property", asFUNCTION(Time::GetUnscaledDeltaTime));
	global("float get_time() property", asFUNCTION(Time::GetTime));
	global("int get_frameCount() property", asFUNCTION(GetFrameCount));
	global("float get_timeScale() property", asFUNCTION(Time::GetTimeScale));
	global("void set_timeScale(float value) property", asFUNCTION(Time::SetTimeScale));
	global("float get_fixedDeltaTime() property", asFUNCTION(Time::GetFixedDeltaTime));
	global("void set_fixedDeltaTime(float value) property", asFUNCTION(Time::SetFixedDeltaTime));

	engine->SetDefaultNamespace("");
}


void ou::ScriptBindings::RegisterInput(asIScriptEngine *engine) {
	// KeyCode::Space のように書く（AngelScript の enum の書き方）
	Check(engine->RegisterEnum("KeyCode"), "KeyCode");
	for (const auto key : magic_enum::enum_values<KeyCode>()) {
		const auto name = std::string(magic_enum::enum_name(key));
		Check(engine->RegisterEnumValue("KeyCode", name.c_str(), static_cast<int>(key)), name);
	}

	engine->SetDefaultNamespace("Input");

	const auto global = [&](const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterGlobalFunction(declaration, function, asCALL_CDECL), declaration);
	};
	global("bool GetKey(KeyCode key)", asFUNCTION(GetKey));
	global("bool GetKeyDown(KeyCode key)", asFUNCTION(GetKeyDown));
	global("bool GetKeyUp(KeyCode key)", asFUNCTION(GetKeyUp));
	global("bool GetMouseButton(int button)", asFUNCTION(Input::GetMouseButton));
	global("bool GetMouseButtonDown(int button)", asFUNCTION(Input::GetMouseButtonDown));
	global("bool GetMouseButtonUp(int button)", asFUNCTION(Input::GetMouseButtonUp));
	global("Vector3 get_mousePosition() property", asFUNCTION(GetMousePosition));
	global("Vector2 get_mouseScrollDelta() property", asFUNCTION(GetMouseScrollDelta));
	global("float GetAxis(const string &in axisName)", asFUNCTION(GetAxis));
	global("float GetAxisRaw(const string &in axisName)", asFUNCTION(GetAxis));

	engine->SetDefaultNamespace("");
}
