#pragma once

#include "Scene/Component.h"
#include "Graphics/GraphicsTypes.h"


namespace ou {

// 前方宣言
class Graphic;
class RectTransform;
class Transform;

class Canvas final: public Component {
public: // メンバ関数

	void Awake() override;
	void LateUpdate() override;
	void OnDestroy() override;

	void Render(Graphics::CommandList cmd) const;


	private: // メンバ変数

		RectTransform *rectTransform_ = nullptr;


		private: // メンバ関数

			void FitToScreen();

			static void CollectGraphics(const Transform *transform, std::vector<const Graphic *> &graphics);
};

} // namespace ou
