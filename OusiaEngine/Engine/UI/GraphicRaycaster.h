#pragma once

#include "Scene/Behaviour.h"
#include "UI/EventSystem/EventData.h"
#include "Math/Vector2.h"

#include <vector>


namespace ou {

// 前方宣言
class Graphic;
class Transform;

class GraphicRaycaster final: public Behaviour {
public: // 静的メンバ関数

	static const std::vector<GraphicRaycaster *> &GetRaycasters();


public: // メンバ関数

	void OnEnable() override;
	void OnDisable() override;

	void Raycast(const PointerEventData &eventData, std::vector<RaycastResult> &resultAppendList);


private: // 静的メンバ変数

	static inline std::vector<GraphicRaycaster *> sRaycasters_;


private: // 静的メンバ関数

	static void CollectGraphics(const Transform *transform, std::vector<Graphic *> &graphics);

	static bool ContainsCanvasPoint(const Graphic *graphic, const Vector2 &canvasPoint);
};

} // namespace ou
