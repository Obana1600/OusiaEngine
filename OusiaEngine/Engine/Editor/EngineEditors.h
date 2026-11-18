#pragma once

#include "Editor/Editor.h"


// エンジンのコンポーネントの Inspector を日本語で描く Editor（Unity のカスタム Editor）
// 欄は日本語、マウスを乗せると英語の名前（スクリプトで使う名前）が出る。Enum の選択肢は英語のまま、説明をツールチップに出す
// 登録は CustomEditor.cpp の RegisterEngineEditors
namespace ou {

// ---- Scene ----

class TransformEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class RectTransformEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};


// ---- Render ----

class CameraEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class LightEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class MeshRendererEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class SkinnedMeshRendererEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class LineRendererEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class SkeletalAnimatorEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};


// ---- Physics ----

class BoxColliderEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class SphereColliderEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class CapsuleColliderEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class CharacterControllerEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class RigidbodyEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};


// ---- Audio ----

class AudioSourceEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class AudioListenerEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};


// ---- UI ----

class CanvasEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class CanvasRendererEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class GraphicRaycasterEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class ImageEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class TextEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class ButtonEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class EventSystemEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

class StandaloneInputModuleEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};


// ---- Particle ----

class ParticleSystemEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
	void OnSceneGui(const Rect &sceneRect) override; // Scene ビューの右下に Particle Effect のパネルを出す
};

class ParticleSystemRendererEditor final: public Editor {
public: // メンバ関数
	void OnInspectorGui() override;
};

} // namespace ou
