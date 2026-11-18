#include "Gizmos.h"
#include "Math/Mathf.h"
#include "Scene/GameObject.h"

#include <algorithm>
#include <cmath>

using namespace ou;


namespace {

constexpr int kCircleSegments = 32;

// a と直交する、長さ 1 の向きを 1 つ選ぶ
Vector3 FindPerpendicular(const Vector3 &a) {
	const auto other = Mathf::Abs(a.x) < 0.9f ? Vector3::Right : Vector3::Up;
	return Vector3::Cross(a, other).Normalized();
}

} // namespace



void ou::Gizmos::SetColor(const Color &color) {
	sColor_ = color;
}


const Color &ou::Gizmos::GetColor() {
	return sColor_;
}


void ou::Gizmos::SetMatrix(const Matrix4x4 &matrix) {
	sMatrix_ = matrix;
}


const Matrix4x4 &ou::Gizmos::GetMatrix() {
	return sMatrix_;
}


void ou::Gizmos::DrawLine(const Vector3 &from, const Vector3 &to) {
	AddLine(from, to);
}


void ou::Gizmos::DrawRay(const Vector3 &from, const Vector3 &direction) {
	AddLine(from, from + direction);
}


void ou::Gizmos::DrawWireCube(const Vector3 &center, const Vector3 &size) {
	const auto e = size * 0.5f;
	Vector3 corners[8];
	for (int i = 0; i < 8; i++) {
		corners[i] = center + Vector3((i & 1) ? e.x : -e.x, (i & 2) ? e.y : -e.y, (i & 4) ? e.z : -e.z);
	}
	// 1 つの軸だけが違う角どうしをつなぐ（12 本）
	for (int i = 0; i < 8; i++) {
		for (const int bit : {1, 2, 4}) {
			if ((i & bit) == 0) { AddLine(corners[i], corners[i | bit]); }
		}
	}
}


void ou::Gizmos::DrawWireSphere(const Vector3 &center, float radius) {
	AddArc(center, Vector3::Right, Vector3::Up, radius, 0.0f, 360.0f, kCircleSegments);
	AddArc(center, Vector3::Up, Vector3::Forward, radius, 0.0f, 360.0f, kCircleSegments);
	AddArc(center, Vector3::Forward, Vector3::Right, radius, 0.0f, 360.0f, kCircleSegments);
}


void ou::Gizmos::DrawFrustum(const Vector3 &center, float fieldOfView, float maxRange, float minRange, float aspect) {
	const auto tanHalf = Mathf::Tan(fieldOfView * 0.5f * Mathf::Deg2Rad);

	const auto makeRect = [&](float distance, Vector3 (&out)[4]) {
		const auto h = tanHalf * distance;
		const auto w = h * aspect;
		out[0] = center + Vector3(-w, -h, distance);
		out[1] = center + Vector3(w, -h, distance);
		out[2] = center + Vector3(w, h, distance);
		out[3] = center + Vector3(-w, h, distance);
	};

	Vector3 nearRect[4];
	Vector3 farRect[4];
	makeRect(minRange, nearRect);
	makeRect(maxRange, farRect);

	for (int i = 0; i < 4; i++) {
		AddLine(nearRect[i], nearRect[(i + 1) % 4]);
		AddLine(farRect[i], farRect[(i + 1) % 4]);
		AddLine(nearRect[i], farRect[i]);
	}
}


void ou::Gizmos::DrawIcon(const Vector3 &center, std::string_view name, bool allowScaling) {
	DrawIcon(center, name, allowScaling, Color::White);
}


void ou::Gizmos::DrawIcon(const Vector3 &center, std::string_view name, bool allowScaling, const Color &tint) {
	if (!sIsDrawing_ || name.empty()) { return; }
	sIcons_.push_back(Icon{sMatrix_.MultiplyPoint3x4(center), std::string(name), tint, allowScaling, sGameObjectId_});
}


void ou::Gizmos::DrawWireDisc(const Vector3 &center, const Vector3 &normal, float radius) {
	if (normal.LengthSquared() < 1e-12f) { return; }

	const auto n = normal.Normalized();
	const auto axisX = FindPerpendicular(n);
	const auto axisY = Vector3::Cross(n, axisX);
	AddArc(center, axisX, axisY, radius, 0.0f, 360.0f, kCircleSegments);
}


void ou::Gizmos::DrawWireCapsule(const Vector3 &point0, const Vector3 &point1, float radius) {
	const auto offset = point1 - point0;
	const auto axis = offset.LengthSquared() > 1e-12f ? offset.Normalized() : Vector3::Up;
	const auto u = FindPerpendicular(axis);
	const auto v = Vector3::Cross(axis, u);

	// 両端の輪と、それをつなぐ 4 本の線
	AddArc(point0, u, v, radius, 0.0f, 360.0f, kCircleSegments);
	AddArc(point1, u, v, radius, 0.0f, 360.0f, kCircleSegments);
	for (const auto &side : {u, -u, v, -v}) {
		AddLine(point0 + side * radius, point1 + side * radius);
	}

	// 両端の半球（point1 は axis の向き、point0 はその逆に膨らむ）
	AddArc(point1, u, axis, radius, 0.0f, 180.0f, kCircleSegments / 2);
	AddArc(point1, v, axis, radius, 0.0f, 180.0f, kCircleSegments / 2);
	AddArc(point0, u, -axis, radius, 0.0f, 180.0f, kCircleSegments / 2);
	AddArc(point0, v, -axis, radius, 0.0f, 180.0f, kCircleSegments / 2);
}


void ou::Gizmos::Clear() {
	sVertices_.clear();
	sIcons_.clear();
}


void ou::Gizmos::BeginDraw(const GameObject *gameObject) {
	sIsDrawing_ = true;
	sGameObjectId_ = gameObject ? gameObject->GetInstanceId() : 0;
	sColor_ = Color::White;
	sMatrix_ = Matrix4x4::Identity;
}


void ou::Gizmos::EndDraw() {
	sIsDrawing_ = false;
	sGameObjectId_ = 0;
	sColor_ = Color::White;
	sMatrix_ = Matrix4x4::Identity;
}


void ou::Gizmos::AddLine(const Vector3 &from, const Vector3 &to) {
	if (!sIsDrawing_) { return; }

	// 色は sRGB で受け取り、シェーダーにはリニアで渡す（LineRenderer と同じ）
	const auto color = sColor_.Linear();
	sVertices_.push_back(Vertex{sMatrix_.MultiplyPoint3x4(from), 0.0f, color});
	sVertices_.push_back(Vertex{sMatrix_.MultiplyPoint3x4(to), 0.0f, color});
}


void ou::Gizmos::AddArc(const Vector3 &center, const Vector3 &axisX, const Vector3 &axisY, float radius, float startDeg, float endDeg, int segments) {
	if (!sIsDrawing_ || segments <= 0) { return; }

	const auto pointAt = [&](int i) {
		const auto angle = (startDeg + (endDeg - startDeg) * static_cast<float>(i) / static_cast<float>(segments)) * Mathf::Deg2Rad;
		return center + (axisX * Mathf::Cos(angle) + axisY * Mathf::Sin(angle)) * radius;
	};

	auto previous = pointAt(0);
	for (int i = 1; i <= segments; i++) {
		const auto current = pointAt(i);
		AddLine(previous, current);
		previous = current;
	}
}
