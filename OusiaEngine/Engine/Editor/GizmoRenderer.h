#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Math/Rect.h"
#include "Render/Camera.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>


struct ImDrawList;


namespace ou {

// 前方宣言
class GameObject;
class Scene;
class Shader;
class Texture;

// Scene ビューに Gizmos を描く（エディタだけ）
// 1. SceneView::OnGui で Collect を呼び、コンポーネントの OnDrawGizmos / OnDrawGizmosSelected に描かせる
// 2. 同じく DrawIcons で、アイコンを ImGui で Scene ビューの絵の上に描く（物に隠れない）
// 3. Engine::RenderSceneView で Render を呼び、線を Scene ビューの深度を使って描く（物の裏は薄く）
class GizmoRenderer final {
	friend class Engine;

public: // 静的メンバ関数

	GizmoRenderer() = delete;

	// Scene ビューのツールバーの Gizmos ボタン
	static void SetEnabled(bool isEnabled);
	static bool IsEnabled();

	static void Collect(Scene *scene);

	// sceneRect は Scene ビューの絵の、画面上の場所
	static void DrawIcons(const Camera::PerCamera &camera, const Rect &sceneRect, ImDrawList *drawList);


private: // インナークラス

	// Gizmo.hlsli の PerGizmo と同じ並び
	struct PerGizmo {
		uint32_t vertexBufferIndex = 0;
		float alphaScale = 1.0f;
		float padding[2] = {};
	};


private: // 静的メンバ変数

	static inline bool sIsEnabled_ = true;

	static inline std::shared_ptr<Shader> sShader_;
	static inline Graphics::GpuBuffer sVertexBuffers_[Graphics::GraphicsDevice::BUFFER_COUNT];
	static inline uint32_t sCapacity_ = 0;

	// アイコンの名前から画像を引く（見つからなかったものは nullptr を入れて、毎フレーム探さない）
	static inline std::unordered_map<std::string, std::shared_ptr<Texture>> sIconTextures_;


private: // 静的メンバ関数

	static void StaticInitialize();
	static void StaticFinalize();

	static void Render(const Camera::PerCamera &camera, Graphics::CommandList cmd);

	// gameObject と子の OnDrawGizmos を呼ぶ
	static void DrawGameObject(GameObject *gameObject, const GameObject *selected, bool isParentSelected);

	static void EnsureCapacity(uint32_t count);
	static Texture *FindIconTexture(const std::string &name);
};

} // namespace ou
