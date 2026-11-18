#pragma once

#include "Math/Color.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>


namespace ou {

// 前方宣言
class GameObject;

// Unity の Gizmos。Scene ビューに、見るだけの線とアイコンを描く
// Unity と同じく、Component::OnDrawGizmos / OnDrawGizmosSelected の中でだけ描ける（ほかで呼んでも何も出ない）
// 描いたものはためておくだけで、エディタの GizmoRenderer が Scene ビューに描く
class Gizmos final {
	friend class GizmoRenderer;

public: // インナークラス

	// GPU に送る線の頂点（Gizmo.hlsli の GizmoVertex と同じ並び）
	struct Vertex {
		Vector3 position;
		float padding = 0.0f;
		Color color;
	};

	struct Icon {
		Vector3 position;
		std::string name;
		Color tint;
		bool allowScaling = true;
		uint32_t gameObjectId = 0; // クリックで選ぶときに使う
	};


public: // 静的メンバ関数

	Gizmos() = delete;

	// 次に描くものの色（OnDrawGizmos を呼ぶたびに白に戻る）
	static void SetColor(const Color &color);
	static const Color &GetColor();

	// 次に描くものに掛ける行列（OnDrawGizmos を呼ぶたびに単位行列に戻る）
	static void SetMatrix(const Matrix4x4 &matrix);
	static const Matrix4x4 &GetMatrix();

	static void DrawLine(const Vector3 &from, const Vector3 &to);
	static void DrawRay(const Vector3 &from, const Vector3 &direction);
	static void DrawWireCube(const Vector3 &center, const Vector3 &size);
	static void DrawWireSphere(const Vector3 &center, float radius);
	// center から +Z を向いた視錐台。fieldOfView は縦の角度（度）
	static void DrawFrustum(const Vector3 &center, float fieldOfView, float maxRange, float minRange, float aspect);

	// アイコンを出す。name は Assets/Gizmos/ か EngineAssets/Gizmos/ の画像の名前（拡張子がなければ .png）
	// 行列は位置にだけ掛ける。アイコンは常にカメラを向き、手前に描く
	static void DrawIcon(const Vector3 &center, std::string_view name, bool allowScaling = true);
	static void DrawIcon(const Vector3 &center, std::string_view name, bool allowScaling, const Color &tint);

	// ---- ここから下は Unity の Gizmos にはない（エンジンの組み込みのギズモで使う） ----

	// normal を軸にした円
	static void DrawWireDisc(const Vector3 &center, const Vector3 &normal, float radius);
	// point0 と point1 を両端の球の中心にしたカプセル
	static void DrawWireCapsule(const Vector3 &point0, const Vector3 &point1, float radius);


private: // 静的メンバ変数

	static inline Color sColor_ = Color::White;
	static inline Matrix4x4 sMatrix_ = Matrix4x4::Identity;

	// OnDrawGizmos の中か
	static inline bool sIsDrawing_ = false;
	static inline uint32_t sGameObjectId_ = 0;

	static inline std::vector<Vertex> sVertices_;
	static inline std::vector<Icon> sIcons_;


private: // 静的メンバ関数

	// GizmoRenderer から呼ぶ。前のフレームの分を捨てる
	static void Clear();
	// GizmoRenderer から呼ぶ。gameObject の OnDrawGizmos を呼ぶ前と後
	static void BeginDraw(const GameObject *gameObject);
	static void EndDraw();

	// 行列を掛けて、今の色で線を足す
	static void AddLine(const Vector3 &from, const Vector3 &to);
	// center + (axisX * cos + axisY * sin) * radius を、startDeg から endDeg までつなぐ
	static void AddArc(const Vector3 &center, const Vector3 &axisX, const Vector3 &axisY, float radius, float startDeg, float endDeg, int segments);
};

} // namespace ou
