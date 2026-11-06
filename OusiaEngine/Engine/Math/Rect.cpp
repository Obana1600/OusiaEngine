#include "Rect.h"
#include "Math/Mathf.h"

#include <format>

using namespace ou;


const Rect ou::Rect::Zero = {0.0f, 0.0f, 0.0f, 0.0f};


ou::Rect::Rect()
	: x(0.0f),
	  y(0.0f),
	  width(0.0f),
	  height(0.0f) {}


ou::Rect::Rect(float x, float y, float width, float height)
	: x(x),
	  y(y),
	  width(width),
	  height(height) {}


ou::Rect::Rect(const Vector2 &position, const Vector2 &size)
	: x(position.x),
	  y(position.y),
	  width(size.x),
	  height(size.y) {}


void ou::Rect::SetPosition(const Vector2 &position) {
	x = position.x;
	y = position.y;
}


Vector2 ou::Rect::GetPosition() const {
	return Vector2(x, y);
}


void ou::Rect::SetSize(const Vector2 &size) {
	width = size.x;
	height = size.y;
}


Vector2 ou::Rect::GetSize() const {
	return Vector2(width, height);
}


void ou::Rect::SetCenter(const Vector2 &center) {
	x = center.x - width * 0.5f;
	y = center.y - height * 0.5f;
}


Vector2 ou::Rect::GetCenter() const {
	return Vector2(x + width * 0.5f, y + height * 0.5f);
}


void ou::Rect::SetMin(const Vector2 &min) {
	SetXMin(min.x);
	SetYMin(min.y);
}


Vector2 ou::Rect::GetMin() const {
	return Vector2(GetXMin(), GetYMin());
}


void ou::Rect::SetMax(const Vector2 &max) {
	SetXMax(max.x);
	SetYMax(max.y);
}


Vector2 ou::Rect::GetMax() const {
	return Vector2(GetXMax(), GetYMax());
}


void ou::Rect::SetXMin(float xMin) {
	const auto xMax = GetXMax();
	x = xMin;
	width = xMax - x;
}


float ou::Rect::GetXMin() const {
	return x;
}


void ou::Rect::SetYMin(float yMin) {
	const auto yMax = GetYMax();
	y = yMin;
	height = yMax - y;
}


float ou::Rect::GetYMin() const {
	return y;
}


void ou::Rect::SetXMax(float xMax) {
	width = xMax - x;
}


float ou::Rect::GetXMax() const {
	return x + width;
}


void ou::Rect::SetYMax(float yMax) {
	height = yMax - y;
}


float ou::Rect::GetYMax() const {
	return y + height;
}


bool ou::Rect::Contains(const Vector2 &point) const {
	return point.x >= GetXMin() && point.x < GetXMax() &&
		   point.y >= GetYMin() && point.y < GetYMax();
}


bool ou::Rect::Overlaps(const Rect &other) const {
	return other.GetXMax() > GetXMin() && other.GetXMin() < GetXMax() &&
		   other.GetYMax() > GetYMin() && other.GetYMin() < GetYMax();
}


std::string ou::Rect::ToString() const {
	return std::format("(x: {:.2f}, y: {:.2f}, width: {:.2f}, height: {:.2f})",
					   x, y, width, height);
}


std::string ou::Rect::ToString(std::string_view format) const {
	const auto fmt = std::format("(x: {{:{0}}}, y: {{:{0}}}, width: {{:{0}}}, height: {{:{0}}})", format);
	return std::vformat(fmt, std::make_format_args(x, y, width, height));
}


Rect ou::Rect::MinMaxRect(float xMin, float yMin, float xMax, float yMax) {
	return Rect(xMin, yMin, xMax - xMin, yMax - yMin);
}


Vector2 ou::Rect::NormalizedToPoint(const Rect &rect, const Vector2 &normalizedCoordinates) {
	return Vector2(Mathf::Lerp(rect.GetXMin(), rect.GetXMax(), normalizedCoordinates.x),
				   Mathf::Lerp(rect.GetYMin(), rect.GetYMax(), normalizedCoordinates.y));
}


Vector2 ou::Rect::PointToNormalized(const Rect &rect, const Vector2 &point) {
	return Vector2(Mathf::InverseLerp(rect.GetXMin(), rect.GetXMax(), point.x),
				   Mathf::InverseLerp(rect.GetYMin(), rect.GetYMax(), point.y));
}


bool ou::operator==(const Rect &a, const Rect &b) {
	return a.x == b.x &&
		   a.y == b.y &&
		   a.width == b.width &&
		   a.height == b.height;
}
