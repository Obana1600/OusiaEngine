#include "ScriptBindings.h"
#include "Math/Bounds.h"
#include "Math/Color.h"
#include "Math/Mathf.h"
#include "Math/Matrix4x4.h"
#include "Math/Quaternion.h"
#include "Math/Rect.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Script/ScriptBindingsInternal.h"

#include <cstring>
#include <format>
#include <new>
#include <string>

#include <angelscript.h>

using namespace ou;
using ScriptBindingsInternal::Check;
using ScriptBindingsInternal::ThrowException;


namespace {

// ---- 演算子（メンバでない operator はアドレスを取れないので、小さな関数で包む） ----

template <typename T> T OpAdd(const T &self, const T &other) { return self + other; }
template <typename T> T OpSub(const T &self, const T &other) { return self - other; }
template <typename T> T OpMul(const T &self, float value) { return self * value; }
template <typename T> T OpMulReverse(const T &self, float value) { return value * self; }
template <typename T> T OpDiv(const T &self, float value) { return self / value; }
template <typename T> T OpNeg(const T &self) { return -self; }
template <typename T> bool OpEquals(const T &self, const T &other) { return self == other; }
template <typename T> T &OpAddAssign(T &self, const T &other) { return self += other; }
template <typename T> T &OpSubAssign(T &self, const T &other) { return self -= other; }
template <typename T> T &OpMulAssign(T &self, float value) { return self *= value; }
template <typename T> T &OpDivAssign(T &self, float value) { return self /= value; }
template <typename T> std::string ToString(const T &self) { return self.ToString(); }

template <typename T> void ConstructDefault(void *memory) { new (memory) T{}; }


// ---- Vector2 / Vector3 ----

void ConstructVector2(float x, float y, void *memory) { new (memory) Vector2(x, y); }
void ConstructVector3(float x, float y, float z, void *memory) { new (memory) Vector3(x, y, z); }
void ConstructVector3XY(float x, float y, void *memory) { new (memory) Vector3(x, y, 0.0f); }

template <typename T> float Magnitude(const T &self) { return self.Length(); }
template <typename T> float SqrMagnitude(const T &self) { return self.LengthSquared(); }
template <typename T> T Normalized(const T &self) { return self.Normalized(); }
template <typename T> T NormalizeStatic(const T &value) { return value.Normalized(); }

Vector3 Vector2ToVector3(const Vector2 &self) { return {self.x, self.y, 0.0f}; }

template <typename T>
T MoveTowards(const T &current, const T &target, float maxDistanceDelta) {
	const auto toTarget = target - current;
	const auto distance = toTarget.Length();
	if (distance <= maxDistanceDelta || distance == 0.0f) { return target; }
	return current + toTarget / distance * maxDistanceDelta;
}

// Unity と同じく度で返す
float AngleVector3(const Vector3 &from, const Vector3 &to) {
	const auto denominator = Mathf::Sqrt(from.LengthSquared() * to.LengthSquared());
	if (denominator < 1e-15f) { return 0.0f; }
	const auto dot = Mathf::Clamp(Vector3::Dot(from, to) / denominator, -1.0f, 1.0f);
	return Mathf::Acos(dot) * Mathf::Rad2Deg;
}

Vector3 ClampMagnitude(const Vector3 &vector, float maxLength) {
	const auto lengthSquared = vector.LengthSquared();
	if (lengthSquared <= maxLength * maxLength) { return vector; }
	return vector / Mathf::Sqrt(lengthSquared) * maxLength;
}


// ---- Quaternion（スクリプトは Unity と同じく度、エンジンはラジアン） ----

void ConstructQuaternion(float x, float y, float z, float w, void *memory) { new (memory) Quaternion(x, y, z, w); }

Quaternion QuaternionMul(const Quaternion &self, const Quaternion &other) { return self * other; }
Vector3 QuaternionMulVector3(const Quaternion &self, const Vector3 &point) { return self * point; }

// Unity と同じく 0〜360 にそろえる
float Wrap360(float degrees) {
	// 0 のわずかに下は Repeat で 360 になってしまうので 0 にする
	const auto wrapped = Mathf::Repeat(degrees, 360.0f);
	return 360.0f - wrapped < 1e-4f ? 0.0f : wrapped;
}

Vector3 ToDegrees360(const Vector3 &radians) {
	const auto degrees = radians * Mathf::Rad2Deg;
	return {Wrap360(degrees.x), Wrap360(degrees.y), Wrap360(degrees.z)};
}

Vector3 GetEulerAngles(const Quaternion &self) { return ToDegrees360(self.EulerAngles()); }
void SetEulerAngles(const Vector3 &degrees, Quaternion &self) { self = Quaternion::Euler(degrees * Mathf::Deg2Rad); }
Quaternion QuaternionNormalized(const Quaternion &self) { return self.Normalized(); }

Quaternion Euler(float x, float y, float z) { return Quaternion::Euler(x * Mathf::Deg2Rad, y * Mathf::Deg2Rad, z * Mathf::Deg2Rad); }
Quaternion EulerVector3(const Vector3 &degrees) { return Quaternion::Euler(degrees * Mathf::Deg2Rad); }
Quaternion AngleAxis(float angle, const Vector3 &axis) { return Quaternion::AngleAxis(axis, angle * Mathf::Deg2Rad); }
float AngleQuaternion(const Quaternion &a, const Quaternion &b) { return Quaternion::Angle(a, b) * Mathf::Rad2Deg; }

// forward を +Z、upwards をおおよその +Y に向ける回転（Unity の Quaternion.LookRotation）
Quaternion LookRotation(const Vector3 &forward, const Vector3 &upwards) {
	if (forward.LengthSquared() < 1e-12f) { return Quaternion::Identity; }

	const auto f = forward.Normalized();
	auto r = Vector3::Cross(upwards, f);
	if (r.LengthSquared() < 1e-12f) {
		// upwards と forward が平行なら、別の軸から right を作る
		r = Vector3::Cross(Mathf::Abs(f.y) < 0.999f ? Vector3::Up : Vector3::Right, f);
	}
	r = r.Normalized();
	const auto u = Vector3::Cross(f, r);

	// 列が right・up・forward の回転行列からクォータニオンにする
	const auto m00 = r.x, m01 = u.x, m02 = f.x;
	const auto m10 = r.y, m11 = u.y, m12 = f.y;
	const auto m20 = r.z, m21 = u.z, m22 = f.z;
	const auto trace = m00 + m11 + m22;

	Quaternion q;
	if (trace > 0.0f) {
		const auto s = 0.5f / Mathf::Sqrt(trace + 1.0f);
		q = {(m21 - m12) * s, (m02 - m20) * s, (m10 - m01) * s, 0.25f / s};
	} else if (m00 > m11 && m00 > m22) {
		const auto s = 2.0f * Mathf::Sqrt(1.0f + m00 - m11 - m22);
		q = {0.25f * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s};
	} else if (m11 > m22) {
		const auto s = 2.0f * Mathf::Sqrt(1.0f + m11 - m00 - m22);
		q = {(m01 + m10) / s, 0.25f * s, (m12 + m21) / s, (m02 - m20) / s};
	} else {
		const auto s = 2.0f * Mathf::Sqrt(1.0f + m22 - m00 - m11);
		q = {(m02 + m20) / s, (m12 + m21) / s, 0.25f * s, (m10 - m01) / s};
	}
	return q.Normalized();
}


// ---- Color ----

void ConstructColor3(float r, float g, float b, void *memory) { new (memory) Color(r, g, b, 1.0f); }
void ConstructColor4(float r, float g, float b, float a, void *memory) { new (memory) Color(r, g, b, a); }
Color ColorMul(const Color &self, const Color &other) { return self * other; }
Color ColorGamma(const Color &self) { return self.Gamma(); }
Color ColorLinear(const Color &self) { return self.Linear(); }
float ColorGrayscale(const Color &self) { return self.GrayScale().r; }
float ColorMaxComponent(const Color &self) { return self.MaxColorComponent(); }
void RGBToHSV(const Color &rgb, float &h, float &s, float &v) { Color::RGBToHSV(rgb, h, s, v); }


// ---- Quaternion の残り ----

Quaternion QuaternionNormalizeStatic(const Quaternion &q) { return q.Normalized(); }

void SetFromToRotation(const Vector3 &fromDirection, const Vector3 &toDirection, Quaternion &self) {
	self.SetFromToRotation(fromDirection, toDirection);
}

// Unity と同じく度で返す
void ToAngleAxis(float &angle, Vector3 &axis, const Quaternion &self) {
	float radians = 0.0f;
	self.ToAngleAxis(&axis, &radians);
	angle = radians * Mathf::Rad2Deg;
}


// ---- Vector4 ----

void ConstructVector4(float x, float y, float z, float w, void *memory) { new (memory) Vector4(x, y, z, w); }
void ConstructVector4XYZ(float x, float y, float z, void *memory) { new (memory) Vector4(x, y, z, 0.0f); }
void ConstructVector4XY(float x, float y, void *memory) { new (memory) Vector4(x, y, 0.0f, 0.0f); }
Vector3 Vector4ToVector3(const Vector4 &self) { return self.XYZ(); }
Vector2 Vector4ToVector2(const Vector4 &self) { return self.XY(); }
Vector4 Vector3ToVector4(const Vector3 &self) { return {self, 0.0f}; }
Vector4 ScaleVector4(const Vector4 &a, const Vector4 &b) { return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w}; }


// ---- Matrix4x4 ----
// エンジンは行ベクトル（平行移動が 4 行目）、Unity は列ベクトル（平行移動が 4 列目）なので、スクリプトから見ると転置になる
// 中身はエンジンの並びのまま持ち、Unity の mRC はエンジンの m[C][R] に読み替える。掛け算の順も逆にする

void ConstructMatrix4x4Columns(const Vector4 &column0, const Vector4 &column1, const Vector4 &column2, const Vector4 &column3, void *memory) {
	// Unity の列はエンジンの行
	new (memory) Matrix4x4(column0, column1, column2, column3);
}

bool CheckMatrixIndex(int index) {
	if (0 <= index && index < 4) { return true; }
	ThrowException(std::format("IndexOutOfRangeException: Invalid matrix index! ({})", index));
	return false;
}

Vector4 GetMatrixColumn(int index, const Matrix4x4 &self) { return CheckMatrixIndex(index) ? self.GetRow(index) : Vector4::Zero; }
Vector4 GetMatrixRow(int index, const Matrix4x4 &self) { return CheckMatrixIndex(index) ? self.GetColumn(index) : Vector4::Zero; }
void SetMatrixColumn(int index, const Vector4 &column, Matrix4x4 &self) { if (CheckMatrixIndex(index)) { self.SetRow(index, column); } }
void SetMatrixRow(int index, const Vector4 &row, Matrix4x4 &self) { if (CheckMatrixIndex(index)) { self.SetColumn(index, row); } }

Matrix4x4 MatrixMul(const Matrix4x4 &self, const Matrix4x4 &other) { return other * self; }

// Unity の M * v（列ベクトル）は、エンジンの並びでは v（行ベクトル）* M
Vector4 MatrixMulVector4(const Matrix4x4 &self, const Vector4 &v) {
	Vector4 result;
	for (int column = 0; column < 4; column++) {
		result[column] = v.x * self.m[0][column] + v.y * self.m[1][column] + v.z * self.m[2][column] + v.w * self.m[3][column];
	}
	return result;
}

bool MatrixEquals(const Matrix4x4 &self, const Matrix4x4 &other) { return std::memcmp(self.m, other.m, sizeof(self.m)) == 0; }
std::string MatrixToString(const Matrix4x4 &self) { return self.Transpose().ToString(); } // Unity の並びで出す
Vector3 MatrixGetPosition(const Matrix4x4 &self) { return self.GetPosition(); }
Quaternion MatrixRotation(const Matrix4x4 &self) { return self.GetRotation(); }
Vector3 MatrixLossyScale(const Matrix4x4 &self) { return self.GetLossyScale(); }
float MatrixDeterminant(const Matrix4x4 &self) { return self.Determinant(); }
Matrix4x4 MatrixInverse(const Matrix4x4 &self) { return self.Inverse(); }
Matrix4x4 MatrixTranspose(const Matrix4x4 &self) { return self.Transpose(); }
bool MatrixIsIdentity(const Matrix4x4 &self) { return self.IsIdentity(); }
Vector3 MatrixMultiplyPoint(const Matrix4x4 &self, const Vector3 &point) { return self.MultiplyPoint(point); }
Vector3 MatrixMultiplyPoint3x4(const Matrix4x4 &self, const Vector3 &point) { return self.MultiplyPoint3x4(point); }
Vector3 MatrixMultiplyVector(const Matrix4x4 &self, const Vector3 &vector) { return self.MultiplyVector(vector); }

Matrix4x4 MatrixTRS(const Vector3 &position, const Quaternion &rotation, const Vector3 &scale) { return Matrix4x4::SRT(scale, rotation, position); }
Matrix4x4 MatrixTranslate(const Vector3 &vector) { return Matrix4x4::Translate(vector); }
Matrix4x4 MatrixScale(const Vector3 &vector) { return Matrix4x4::Scale(vector); }
Matrix4x4 MatrixRotate(const Quaternion &rotation) { return Matrix4x4::Rotate(rotation); }
Matrix4x4 MatrixInverseStatic(const Matrix4x4 &matrix) { return matrix.Inverse(); }
Matrix4x4 MatrixTransposeStatic(const Matrix4x4 &matrix) { return matrix.Transpose(); }


// ---- Rect ----

void ConstructRect(float x, float y, float width, float height, void *memory) { new (memory) Rect(x, y, width, height); }
void ConstructRectVectors(const Vector2 &position, const Vector2 &size, void *memory) { new (memory) Rect(position, size); }
bool RectContains2(const Vector2 &point, const Rect &self) { return self.Contains(point); }
bool RectContains3(const Vector3 &point, const Rect &self) { return self.Contains({point.x, point.y}); }
bool RectOverlaps(const Rect &other, const Rect &self) { return self.Overlaps(other); }
bool RectEquals(const Rect &self, const Rect &other) { return self == other; }


// ---- Bounds ----

void ConstructBounds(const Vector3 &center, const Vector3 &size, void *memory) { new (memory) Bounds(center, size); }
void BoundsSetMinMax(const Vector3 &min, const Vector3 &max, Bounds &self) { self.SetMinMax(min, max); }
bool BoundsContains(const Vector3 &point, const Bounds &self) { return self.Contains(point); }
bool BoundsIntersects(const Bounds &bounds, const Bounds &self) { return self.Intersects(bounds); }
Vector3 BoundsClosestPoint(const Vector3 &point, const Bounds &self) { return self.ClosestPoint(point); }
float BoundsSqrDistance(const Vector3 &point, const Bounds &self) { return self.SqrDistance(point); }
void EncapsulatePoint(const Vector3 &point, Bounds &self) { self.Encapsulate(point); }
void EncapsulateBounds(const Bounds &bounds, Bounds &self) { self.Encapsulate(bounds); }


// ---- C++ のメンバ関数をそのまま呼ぶ手伝い（自分が最後の引数。asCALL_CDECL_OBJLAST） ----

template <typename T, typename R, R (T::*Getter)() const>
std::decay_t<R> ValueGet(const T &self) { return (self.*Getter)(); }

template <typename T, typename V, void (T::*Setter)(V)>
void ValueSet(V value, T &self) { (self.*Setter)(value); }


// ---- 登録の手伝い ----

template <typename T>
void RegisterValueType(asIScriptEngine *engine, const char *name) {
	// 中身は float だけで、コピーは memcpy でよい（asOBJ_POD）。返り値の渡し方は asGetTypeTraits で C++ に合わせる
	Check(engine->RegisterObjectType(name, sizeof(T), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_ALLFLOATS | asGetTypeTraits<T>()), name);
}

struct TypeRegistrar {
	asIScriptEngine *engine;
	const char *type;

	void Constructor(const char *declaration, const asSFuncPtr &function) const {
		Check(engine->RegisterObjectBehaviour(type, asBEHAVE_CONSTRUCT, declaration, function, asCALL_CDECL_OBJLAST), declaration);
	}

	void Property(const char *declaration, int offset) const {
		Check(engine->RegisterObjectProperty(type, declaration, offset), declaration);
	}

	// 関数の最初の引数が自分（asCALL_CDECL_OBJFIRST）
	void Method(const char *declaration, const asSFuncPtr &function) const {
		Check(engine->RegisterObjectMethod(type, declaration, function, asCALL_CDECL_OBJFIRST), declaration);
	}

	// 関数の最後の引数が自分（asCALL_CDECL_OBJLAST）
	void MethodObjLast(const char *declaration, const asSFuncPtr &function) const {
		Check(engine->RegisterObjectMethod(type, declaration, function, asCALL_CDECL_OBJLAST), declaration);
	}

	// Vector3::zero のような定数も、型と同じ名前の名前空間に置く
	void Constant(const char *declaration, const void *value) const {
		engine->SetDefaultNamespace(type);
		Check(engine->RegisterGlobalProperty(declaration, const_cast<void *>(value)), declaration);
		engine->SetDefaultNamespace("");
	}

	// Vector3::zero のような静的なものは、型と同じ名前の名前空間に置く
	void Static(const char *declaration, const asSFuncPtr &function) const {
		engine->SetDefaultNamespace(type);
		Check(engine->RegisterGlobalFunction(declaration, function, asCALL_CDECL), declaration);
		engine->SetDefaultNamespace("");
	}

	template <typename T>
	void Arithmetic() const {
		const auto name = std::string(type);
		const auto declare = [&](const std::string &format) {
			std::string text = format;
			for (size_t at = text.find('@'); at != std::string::npos; at = text.find('@')) { text.replace(at, 1, name); }
			return text;
		};
		Method(declare("@ opAdd(const @ &in) const").c_str(), asFUNCTION(OpAdd<T>));
		Method(declare("@ opSub(const @ &in) const").c_str(), asFUNCTION(OpSub<T>));
		Method(declare("@ opMul(float) const").c_str(), asFUNCTION(OpMul<T>));
		Method(declare("@ opMul_r(float) const").c_str(), asFUNCTION(OpMulReverse<T>));
		Method(declare("@ opDiv(float) const").c_str(), asFUNCTION(OpDiv<T>));
		Method(declare("bool opEquals(const @ &in) const").c_str(), asFUNCTION(OpEquals<T>));
		Method(declare("@ &opAddAssign(const @ &in)").c_str(), asFUNCTION(OpAddAssign<T>));
		Method(declare("@ &opSubAssign(const @ &in)").c_str(), asFUNCTION(OpSubAssign<T>));
		Method(declare("@ &opMulAssign(float)").c_str(), asFUNCTION(OpMulAssign<T>));
		Method(declare("@ &opDivAssign(float)").c_str(), asFUNCTION(OpDivAssign<T>));
		Method("string ToString() const", asFUNCTION(ToString<T>));
	}
};


} // namespace


Quaternion ou::ScriptBindingsInternal::LookRotation(const Vector3 &forward, const Vector3 &upwards) {
	return ::LookRotation(forward, upwards);
}



void ou::ScriptBindings::RegisterValueTypes(asIScriptEngine *engine) {
	RegisterValueType<Vector2>(engine, "Vector2");
	RegisterValueType<Vector3>(engine, "Vector3");
	RegisterValueType<Quaternion>(engine, "Quaternion");
	RegisterValueType<Color>(engine, "Color");
	RegisterValueType<Vector4>(engine, "Vector4");
	RegisterValueType<Matrix4x4>(engine, "Matrix4x4");
	RegisterValueType<Rect>(engine, "Rect");
	RegisterValueType<Bounds>(engine, "Bounds");

	// ---- Vector2 ----
	{
		const TypeRegistrar v{engine, "Vector2"};
		v.Constructor("void f()", asFUNCTION(ConstructDefault<Vector2>));
		v.Constructor("void f(float x, float y)", asFUNCTION(ConstructVector2));
		v.Property("float x", asOFFSET(Vector2, x));
		v.Property("float y", asOFFSET(Vector2, y));
		v.Arithmetic<Vector2>();
		v.Method("Vector2 opNeg() const", asFUNCTION(OpNeg<Vector2>));
		v.Method("float get_magnitude() const property", asFUNCTION(Magnitude<Vector2>));
		v.Method("float get_sqrMagnitude() const property", asFUNCTION(SqrMagnitude<Vector2>));
		v.Method("Vector2 get_normalized() const property", asFUNCTION(Normalized<Vector2>));
		// Unity と同じく Vector2 から Vector3 へは暗黙に変えられる
		v.Method("Vector3 opImplConv() const", asFUNCTION(Vector2ToVector3));

		v.Constant("const Vector2 zero", &Vector2::Zero);
		v.Constant("const Vector2 one", &Vector2::One);
		v.Constant("const Vector2 up", &Vector2::Up);
		v.Constant("const Vector2 down", &Vector2::Down);
		v.Constant("const Vector2 left", &Vector2::Left);
		v.Constant("const Vector2 right", &Vector2::Right);
		v.Static("float Dot(const Vector2 &in lhs, const Vector2 &in rhs)", asFUNCTION(Vector2::Dot));
		v.Static("float Distance(const Vector2 &in a, const Vector2 &in b)", asFUNCTION(Vector2::Distance));
		v.Static("Vector2 Lerp(const Vector2 &in a, const Vector2 &in b, float t)", asFUNCTION(Vector2::Lerp));
		v.Static("Vector2 LerpUnclamped(const Vector2 &in a, const Vector2 &in b, float t)", asFUNCTION(Vector2::LerpUnclamped));
		v.Static("Vector2 MoveTowards(const Vector2 &in current, const Vector2 &in target, float maxDistanceDelta)", asFUNCTION(MoveTowards<Vector2>));
		v.Static("Vector2 Scale(const Vector2 &in a, const Vector2 &in b)", asFUNCTION(Vector2::Scale));
		v.Static("Vector2 Min(const Vector2 &in lhs, const Vector2 &in rhs)", asFUNCTION(Vector2::Min));
		v.Static("Vector2 Max(const Vector2 &in lhs, const Vector2 &in rhs)", asFUNCTION(Vector2::Max));
	}

	// ---- Vector3 ----
	{
		const TypeRegistrar v{engine, "Vector3"};
		v.Constructor("void f()", asFUNCTION(ConstructDefault<Vector3>));
		v.Constructor("void f(float x, float y, float z)", asFUNCTION(ConstructVector3));
		v.Constructor("void f(float x, float y)", asFUNCTION(ConstructVector3XY));
		v.Property("float x", asOFFSET(Vector3, x));
		v.Property("float y", asOFFSET(Vector3, y));
		v.Property("float z", asOFFSET(Vector3, z));
		v.Arithmetic<Vector3>();
		v.Method("Vector3 opNeg() const", asFUNCTION(OpNeg<Vector3>));
		v.Method("float get_magnitude() const property", asFUNCTION(Magnitude<Vector3>));
		v.Method("float get_sqrMagnitude() const property", asFUNCTION(SqrMagnitude<Vector3>));
		v.Method("Vector3 get_normalized() const property", asFUNCTION(Normalized<Vector3>));

		v.Constant("const Vector3 zero", &Vector3::Zero);
		v.Constant("const Vector3 one", &Vector3::One);
		v.Constant("const Vector3 up", &Vector3::Up);
		v.Constant("const Vector3 down", &Vector3::Down);
		v.Constant("const Vector3 left", &Vector3::Left);
		v.Constant("const Vector3 right", &Vector3::Right);
		v.Constant("const Vector3 forward", &Vector3::Forward);
		v.Constant("const Vector3 back", &Vector3::Back);
		v.Static("float Dot(const Vector3 &in lhs, const Vector3 &in rhs)", asFUNCTION(Vector3::Dot));
		v.Static("Vector3 Cross(const Vector3 &in lhs, const Vector3 &in rhs)", asFUNCTION(Vector3::Cross));
		v.Static("float Distance(const Vector3 &in a, const Vector3 &in b)", asFUNCTION(Vector3::Distance));
		v.Static("float Angle(const Vector3 &in from, const Vector3 &in to)", asFUNCTION(AngleVector3));
		v.Static("Vector3 Lerp(const Vector3 &in a, const Vector3 &in b, float t)", asFUNCTION(Vector3::Lerp));
		v.Static("Vector3 LerpUnclamped(const Vector3 &in a, const Vector3 &in b, float t)", asFUNCTION(Vector3::LerpUnclamped));
		v.Static("Vector3 MoveTowards(const Vector3 &in current, const Vector3 &in target, float maxDistanceDelta)", asFUNCTION(MoveTowards<Vector3>));
		v.Static("Vector3 Normalize(const Vector3 &in value)", asFUNCTION(NormalizeStatic<Vector3>));
		v.Static("Vector3 ClampMagnitude(const Vector3 &in vector, float maxLength)", asFUNCTION(ClampMagnitude));
		v.Static("Vector3 Scale(const Vector3 &in a, const Vector3 &in b)", asFUNCTION(Vector3::Scale));
		v.Static("Vector3 Min(const Vector3 &in lhs, const Vector3 &in rhs)", asFUNCTION(Vector3::Min));
		v.Static("Vector3 Max(const Vector3 &in lhs, const Vector3 &in rhs)", asFUNCTION(Vector3::Max));
	}

	// ---- Quaternion ----
	{
		const TypeRegistrar q{engine, "Quaternion"};
		q.Constructor("void f()", asFUNCTION(ConstructDefault<Quaternion>));
		q.Constructor("void f(float x, float y, float z, float w)", asFUNCTION(ConstructQuaternion));
		q.Property("float x", asOFFSET(Quaternion, x));
		q.Property("float y", asOFFSET(Quaternion, y));
		q.Property("float z", asOFFSET(Quaternion, z));
		q.Property("float w", asOFFSET(Quaternion, w));
		q.Method("Quaternion opMul(const Quaternion &in) const", asFUNCTION(QuaternionMul));
		q.Method("Vector3 opMul(const Vector3 &in) const", asFUNCTION(QuaternionMulVector3));
		q.Method("bool opEquals(const Quaternion &in) const", asFUNCTION(OpEquals<Quaternion>));
		q.Method("string ToString() const", asFUNCTION(ToString<Quaternion>));
		q.Method("Vector3 get_eulerAngles() const property", asFUNCTION(GetEulerAngles));
		q.MethodObjLast("void set_eulerAngles(const Vector3 &in value) property", asFUNCTION(SetEulerAngles));
		q.Method("Quaternion get_normalized() const property", asFUNCTION(QuaternionNormalized));

		q.Constant("const Quaternion identity", &Quaternion::Identity);
		q.Static("Quaternion Euler(float x, float y, float z)", asFUNCTION(Euler));
		q.Static("Quaternion Euler(const Vector3 &in euler)", asFUNCTION(EulerVector3));
		q.Static("Quaternion AngleAxis(float angle, const Vector3 &in axis)", asFUNCTION(AngleAxis));
		q.Static("float Angle(const Quaternion &in a, const Quaternion &in b)", asFUNCTION(AngleQuaternion));
		q.Static("float Dot(const Quaternion &in a, const Quaternion &in b)", asFUNCTION(Quaternion::Dot));
		q.Static("Quaternion Inverse(const Quaternion &in rotation)", asFUNCTION(Quaternion::Inverse));
		q.Static("Quaternion Lerp(const Quaternion &in a, const Quaternion &in b, float t)", asFUNCTION(Quaternion::Lerp));
		q.Static("Quaternion Slerp(const Quaternion &in a, const Quaternion &in b, float t)", asFUNCTION(Quaternion::Slerp));
		q.Static("Quaternion FromToRotation(const Vector3 &in fromDirection, const Vector3 &in toDirection)", asFUNCTION(Quaternion::FromToRotation));
		q.Static("Quaternion LookRotation(const Vector3 &in forward, const Vector3 &in upwards = Vector3::up)", asFUNCTION(LookRotation));
		q.Static("Quaternion LerpUnclamped(const Quaternion &in a, const Quaternion &in b, float t)", asFUNCTION(Quaternion::LerpUnclamped));
		q.Static("Quaternion SlerpUnclamped(const Quaternion &in a, const Quaternion &in b, float t)", asFUNCTION(Quaternion::SlerpUnclamped));
		q.Static("Quaternion Normalize(const Quaternion &in q)", asFUNCTION(QuaternionNormalizeStatic));
		q.MethodObjLast("void SetFromToRotation(const Vector3 &in fromDirection, const Vector3 &in toDirection)", asFUNCTION(SetFromToRotation));
		q.MethodObjLast("void ToAngleAxis(float &out angle, Vector3 &out axis) const", asFUNCTION(ToAngleAxis));
	}

	// ---- Color ----
	{
		const TypeRegistrar c{engine, "Color"};
		c.Constructor("void f()", asFUNCTION(ConstructDefault<Color>));
		c.Constructor("void f(float r, float g, float b)", asFUNCTION(ConstructColor3));
		c.Constructor("void f(float r, float g, float b, float a)", asFUNCTION(ConstructColor4));
		c.Property("float r", asOFFSET(Color, r));
		c.Property("float g", asOFFSET(Color, g));
		c.Property("float b", asOFFSET(Color, b));
		c.Property("float a", asOFFSET(Color, a));
		c.Arithmetic<Color>();
		c.Method("Color opMul(const Color &in) const", asFUNCTION(ColorMul));

		c.Constant("const Color white", &Color::White);
		c.Constant("const Color black", &Color::Black);
		c.Constant("const Color clear", &Color::Clear);
		c.Constant("const Color red", &Color::Red);
		c.Constant("const Color green", &Color::Green);
		c.Constant("const Color blue", &Color::Blue);
		c.Constant("const Color yellow", &Color::Yellow);
		c.Constant("const Color cyan", &Color::Cyan);
		c.Constant("const Color magenta", &Color::Magenta);
		c.Constant("const Color gray", &Color::Gray);
		c.Static("Color Lerp(const Color &in a, const Color &in b, float t)", asFUNCTION(Color::Lerp));
		c.Static("Color LerpUnclamped(const Color &in a, const Color &in b, float t)", asFUNCTION(Color::LerpUnclamped));
		c.Static("Color HSVToRGB(float h, float s, float v)", asFUNCTION(Color::HSVToRGB));
		c.Static("void RGBToHSV(const Color &in rgbColor, float &out H, float &out S, float &out V)", asFUNCTION(RGBToHSV));
		c.Method("Color get_gamma() const property", asFUNCTION(ColorGamma));
		c.Method("Color get_linear() const property", asFUNCTION(ColorLinear));
		c.Method("float get_grayscale() const property", asFUNCTION(ColorGrayscale));
		c.Method("float get_maxColorComponent() const property", asFUNCTION(ColorMaxComponent));
	}

	// ---- Vector4 ----
	{
		const TypeRegistrar v{engine, "Vector4"};
		v.Constructor("void f()", asFUNCTION(ConstructDefault<Vector4>));
		v.Constructor("void f(float x, float y, float z, float w)", asFUNCTION(ConstructVector4));
		v.Constructor("void f(float x, float y, float z)", asFUNCTION(ConstructVector4XYZ));
		v.Constructor("void f(float x, float y)", asFUNCTION(ConstructVector4XY));
		v.Property("float x", asOFFSET(Vector4, x));
		v.Property("float y", asOFFSET(Vector4, y));
		v.Property("float z", asOFFSET(Vector4, z));
		v.Property("float w", asOFFSET(Vector4, w));
		v.Arithmetic<Vector4>();
		v.Method("Vector4 opNeg() const", asFUNCTION(OpNeg<Vector4>));
		v.Method("float get_magnitude() const property", asFUNCTION(Magnitude<Vector4>));
		v.Method("float get_sqrMagnitude() const property", asFUNCTION(SqrMagnitude<Vector4>));
		v.Method("Vector4 get_normalized() const property", asFUNCTION(Normalized<Vector4>));
		// Unity と同じく Vector3・Vector2 と暗黙に変えられる
		v.Method("Vector3 opImplConv() const", asFUNCTION(Vector4ToVector3));
		v.Method("Vector2 opImplConv() const", asFUNCTION(Vector4ToVector2));
		{
			const TypeRegistrar v3{engine, "Vector3"};
			v3.Method("Vector4 opImplConv() const", asFUNCTION(Vector3ToVector4));
		}

		v.Constant("const Vector4 zero", &Vector4::Zero);
		v.Constant("const Vector4 one", &Vector4::One);
		v.Constant("const Vector4 positiveInfinity", &Vector4::PositiveInifinity);
		v.Constant("const Vector4 negativeInfinity", &Vector4::NegativeInifinity);
		v.Static("float Dot(const Vector4 &in a, const Vector4 &in b)", asFUNCTION(Vector4::Dot));
		v.Static("float Distance(const Vector4 &in a, const Vector4 &in b)", asFUNCTION(Vector4::Distance));
		v.Static("Vector4 Lerp(const Vector4 &in a, const Vector4 &in b, float t)", asFUNCTION(Vector4::Lerp));
		v.Static("Vector4 LerpUnclamped(const Vector4 &in a, const Vector4 &in b, float t)", asFUNCTION(Vector4::LerpUnclamped));
		v.Static("Vector4 MoveTowards(const Vector4 &in current, const Vector4 &in target, float maxDistanceDelta)", asFUNCTION(MoveTowards<Vector4>));
		v.Static("Vector4 Normalize(const Vector4 &in a)", asFUNCTION(NormalizeStatic<Vector4>));
		v.Static("Vector4 Scale(const Vector4 &in a, const Vector4 &in b)", asFUNCTION(ScaleVector4));
		v.Static("Vector4 Min(const Vector4 &in lhs, const Vector4 &in rhs)", asFUNCTION(Vector4::Min));
		v.Static("Vector4 Max(const Vector4 &in lhs, const Vector4 &in rhs)", asFUNCTION(Vector4::Max));
	}

	// ---- Matrix4x4 ----
	{
		const TypeRegistrar m{engine, "Matrix4x4"};
		m.Constructor("void f()", asFUNCTION(ConstructDefault<Matrix4x4>));
		m.Constructor("void f(const Vector4 &in column0, const Vector4 &in column1, const Vector4 &in column2, const Vector4 &in column3)", asFUNCTION(ConstructMatrix4x4Columns));
		// Unity の mRC（R 行 C 列）は、エンジンの m[C][R]
		for (int row = 0; row < 4; row++) {
			for (int column = 0; column < 4; column++) {
				const auto declaration = std::format("float m{}{}", row, column);
				m.Property(declaration.c_str(), static_cast<int>(asOFFSET(Matrix4x4, m) + (column * 4 + row) * sizeof(float)));
			}
		}
		m.MethodObjLast("Vector4 GetColumn(int index) const", asFUNCTION(GetMatrixColumn));
		m.MethodObjLast("Vector4 GetRow(int index) const", asFUNCTION(GetMatrixRow));
		m.MethodObjLast("void SetColumn(int index, const Vector4 &in column)", asFUNCTION(SetMatrixColumn));
		m.MethodObjLast("void SetRow(int index, const Vector4 &in row)", asFUNCTION(SetMatrixRow));
		m.Method("Matrix4x4 opMul(const Matrix4x4 &in) const", asFUNCTION(MatrixMul));
		m.Method("Vector4 opMul(const Vector4 &in) const", asFUNCTION(MatrixMulVector4));
		m.Method("bool opEquals(const Matrix4x4 &in) const", asFUNCTION(MatrixEquals));
		m.Method("string ToString() const", asFUNCTION(MatrixToString));
		m.Method("Vector3 GetPosition() const", asFUNCTION(MatrixGetPosition));
		m.Method("Quaternion get_rotation() const property", asFUNCTION(MatrixRotation));
		m.Method("Vector3 get_lossyScale() const property", asFUNCTION(MatrixLossyScale));
		m.Method("float get_determinant() const property", asFUNCTION(MatrixDeterminant));
		m.Method("Matrix4x4 get_inverse() const property", asFUNCTION(MatrixInverse));
		m.Method("Matrix4x4 get_transpose() const property", asFUNCTION(MatrixTranspose));
		m.Method("bool get_isIdentity() const property", asFUNCTION(MatrixIsIdentity));
		m.Method("Vector3 MultiplyPoint(const Vector3 &in point) const", asFUNCTION(MatrixMultiplyPoint));
		m.Method("Vector3 MultiplyPoint3x4(const Vector3 &in point) const", asFUNCTION(MatrixMultiplyPoint3x4));
		m.Method("Vector3 MultiplyVector(const Vector3 &in vector) const", asFUNCTION(MatrixMultiplyVector));

		m.Constant("const Matrix4x4 identity", &Matrix4x4::Identity);
		m.Constant("const Matrix4x4 zero", &Matrix4x4::Zero);
		m.Static("Matrix4x4 TRS(const Vector3 &in pos, const Quaternion &in q, const Vector3 &in s)", asFUNCTION(MatrixTRS));
		m.Static("Matrix4x4 Translate(const Vector3 &in vector)", asFUNCTION(MatrixTranslate));
		m.Static("Matrix4x4 Scale(const Vector3 &in vector)", asFUNCTION(MatrixScale));
		m.Static("Matrix4x4 Rotate(const Quaternion &in q)", asFUNCTION(MatrixRotate));
		m.Static("Matrix4x4 Inverse(const Matrix4x4 &in m)", asFUNCTION(MatrixInverseStatic));
		m.Static("Matrix4x4 Transpose(const Matrix4x4 &in m)", asFUNCTION(MatrixTransposeStatic));
	}

	// ---- Rect ----
	{
		const TypeRegistrar r{engine, "Rect"};
		r.Constructor("void f()", asFUNCTION(ConstructDefault<Rect>));
		r.Constructor("void f(float x, float y, float width, float height)", asFUNCTION(ConstructRect));
		r.Constructor("void f(const Vector2 &in position, const Vector2 &in size)", asFUNCTION(ConstructRectVectors));
		r.Property("float x", asOFFSET(Rect, x));
		r.Property("float y", asOFFSET(Rect, y));
		r.Property("float width", asOFFSET(Rect, width));
		r.Property("float height", asOFFSET(Rect, height));
		r.Method("Vector2 get_position() const property", asFUNCTION((ValueGet<Rect, Vector2, &Rect::GetPosition>)));
		r.MethodObjLast("void set_position(const Vector2 &in value) property", asFUNCTION((ValueSet<Rect, const Vector2 &, &Rect::SetPosition>)));
		r.Method("Vector2 get_size() const property", asFUNCTION((ValueGet<Rect, Vector2, &Rect::GetSize>)));
		r.MethodObjLast("void set_size(const Vector2 &in value) property", asFUNCTION((ValueSet<Rect, const Vector2 &, &Rect::SetSize>)));
		r.Method("Vector2 get_center() const property", asFUNCTION((ValueGet<Rect, Vector2, &Rect::GetCenter>)));
		r.MethodObjLast("void set_center(const Vector2 &in value) property", asFUNCTION((ValueSet<Rect, const Vector2 &, &Rect::SetCenter>)));
		r.Method("Vector2 get_min() const property", asFUNCTION((ValueGet<Rect, Vector2, &Rect::GetMin>)));
		r.MethodObjLast("void set_min(const Vector2 &in value) property", asFUNCTION((ValueSet<Rect, const Vector2 &, &Rect::SetMin>)));
		r.Method("Vector2 get_max() const property", asFUNCTION((ValueGet<Rect, Vector2, &Rect::GetMax>)));
		r.MethodObjLast("void set_max(const Vector2 &in value) property", asFUNCTION((ValueSet<Rect, const Vector2 &, &Rect::SetMax>)));
		r.Method("float get_xMin() const property", asFUNCTION((ValueGet<Rect, float, &Rect::GetXMin>)));
		r.MethodObjLast("void set_xMin(float value) property", asFUNCTION((ValueSet<Rect, float, &Rect::SetXMin>)));
		r.Method("float get_yMin() const property", asFUNCTION((ValueGet<Rect, float, &Rect::GetYMin>)));
		r.MethodObjLast("void set_yMin(float value) property", asFUNCTION((ValueSet<Rect, float, &Rect::SetYMin>)));
		r.Method("float get_xMax() const property", asFUNCTION((ValueGet<Rect, float, &Rect::GetXMax>)));
		r.MethodObjLast("void set_xMax(float value) property", asFUNCTION((ValueSet<Rect, float, &Rect::SetXMax>)));
		r.Method("float get_yMax() const property", asFUNCTION((ValueGet<Rect, float, &Rect::GetYMax>)));
		r.MethodObjLast("void set_yMax(float value) property", asFUNCTION((ValueSet<Rect, float, &Rect::SetYMax>)));
		r.MethodObjLast("bool Contains(const Vector2 &in point) const", asFUNCTION(RectContains2));
		r.MethodObjLast("bool Contains(const Vector3 &in point) const", asFUNCTION(RectContains3));
		r.MethodObjLast("bool Overlaps(const Rect &in other) const", asFUNCTION(RectOverlaps));
		r.Method("bool opEquals(const Rect &in) const", asFUNCTION(RectEquals));
		r.Method("string ToString() const", asFUNCTION(ToString<Rect>));

		r.Constant("const Rect zero", &Rect::Zero);
		r.Static("Rect MinMaxRect(float xmin, float ymin, float xmax, float ymax)", asFUNCTION(Rect::MinMaxRect));
		r.Static("Vector2 NormalizedToPoint(const Rect &in rectangle, const Vector2 &in normalizedRectCoordinates)", asFUNCTION(Rect::NormalizedToPoint));
		r.Static("Vector2 PointToNormalized(const Rect &in rectangle, const Vector2 &in point)", asFUNCTION(Rect::PointToNormalized));
	}

	// ---- Bounds ----
	{
		const TypeRegistrar b{engine, "Bounds"};
		b.Constructor("void f()", asFUNCTION(ConstructDefault<Bounds>));
		b.Constructor("void f(const Vector3 &in center, const Vector3 &in size)", asFUNCTION(ConstructBounds));
		b.Property("Vector3 center", asOFFSET(Bounds, center));
		b.Property("Vector3 extents", asOFFSET(Bounds, extents));
		b.Method("Vector3 get_size() const property", asFUNCTION((ValueGet<Bounds, Vector3, &Bounds::GetSize>)));
		b.MethodObjLast("void set_size(const Vector3 &in value) property", asFUNCTION((ValueSet<Bounds, const Vector3 &, &Bounds::SetSize>)));
		b.Method("Vector3 get_min() const property", asFUNCTION((ValueGet<Bounds, Vector3, &Bounds::GetMin>)));
		b.MethodObjLast("void set_min(const Vector3 &in value) property", asFUNCTION((ValueSet<Bounds, const Vector3 &, &Bounds::SetMin>)));
		b.Method("Vector3 get_max() const property", asFUNCTION((ValueGet<Bounds, Vector3, &Bounds::GetMax>)));
		b.MethodObjLast("void set_max(const Vector3 &in value) property", asFUNCTION((ValueSet<Bounds, const Vector3 &, &Bounds::SetMax>)));
		b.MethodObjLast("void SetMinMax(const Vector3 &in min, const Vector3 &in max)", asFUNCTION(BoundsSetMinMax));
		b.MethodObjLast("bool Contains(const Vector3 &in point) const", asFUNCTION(BoundsContains));
		b.MethodObjLast("bool Intersects(const Bounds &in bounds) const", asFUNCTION(BoundsIntersects));
		b.MethodObjLast("Vector3 ClosestPoint(const Vector3 &in point) const", asFUNCTION(BoundsClosestPoint));
		b.MethodObjLast("float SqrDistance(const Vector3 &in point) const", asFUNCTION(BoundsSqrDistance));
		b.MethodObjLast("void Encapsulate(const Vector3 &in point)", asFUNCTION(EncapsulatePoint));
		b.MethodObjLast("void Encapsulate(const Bounds &in bounds)", asFUNCTION(EncapsulateBounds));
		b.MethodObjLast("void Expand(float amount)", asFUNCTION((ValueSet<Bounds, float, &Bounds::Expand>)));
		b.Method("string ToString() const", asFUNCTION(ToString<Bounds>));
	}
}
