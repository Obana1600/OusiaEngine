#include "CustomEditor.h"
#include "Core/Object.h"
#include "Editor/AssetEditors.h"
#include "Editor/Editor.h"
#include "Editor/EngineEditors.h"

#include "Animation/Skeletal/SkeletalAnimator.h"
#include "Audio/AudioClip.h"
#include "Audio/AudioListener.h"
#include "Audio/AudioSource.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemRenderer.h"
#include "Physics/BoxCollider.h"
#include "Physics/CapsuleCollider.h"
#include "Physics/CharacterController.h"
#include "Physics/PhysicsMaterial.h"
#include "Physics/Rigidbody.h"
#include "Physics/SphereCollider.h"
#include "PostProcess/VolumeProfile.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/Texture.h"
#include "Render/Camera.h"
#include "Render/Light.h"
#include "Render/LineRenderer.h"
#include "Render/MeshRenderer.h"
#include "Render/SkinnedMeshRenderer.h"
#include "Scene/Transform.h"
#include "UI/Button.h"
#include "UI/Canvas.h"
#include "UI/CanvasRenderer.h"
#include "UI/EventSystem/EventSystem.h"
#include "UI/EventSystem/StandaloneInputModule.h"
#include "UI/GraphicRaycaster.h"
#include "UI/Image.h"
#include "UI/RectTransform.h"
#include "UI/Text.h"

using namespace ou;


void ou::CustomEditor::RegisterEngineEditors() {
	// エンジンのコンポーネントを日本語で描く（EngineEditors.h）。MonoBehavior はスクリプトのフィールドなので、全部の値を並べるまま
	Register<TransformEditor, Transform>();
	Register<RectTransformEditor, RectTransform>();

	Register<CameraEditor, Camera>();
	Register<LightEditor, Light>();
	Register<MeshRendererEditor, MeshRenderer>();
	Register<SkinnedMeshRendererEditor, SkinnedMeshRenderer>();
	Register<LineRendererEditor, LineRenderer>();
	Register<SkeletalAnimatorEditor, SkeletalAnimator>();

	Register<BoxColliderEditor, BoxCollider>();
	Register<SphereColliderEditor, SphereCollider>();
	Register<CapsuleColliderEditor, CapsuleCollider>();
	Register<CharacterControllerEditor, CharacterController>();
	Register<RigidbodyEditor, Rigidbody>();

	Register<AudioSourceEditor, AudioSource>();
	Register<AudioListenerEditor, AudioListener>();

	Register<CanvasEditor, Canvas>();
	Register<CanvasRendererEditor, CanvasRenderer>();
	Register<GraphicRaycasterEditor, GraphicRaycaster>();
	Register<ImageEditor, Image>();
	Register<TextEditor, Text>();
	Register<ButtonEditor, Button>();
	Register<EventSystemEditor, EventSystem>();
	Register<StandaloneInputModuleEditor, StandaloneInputModule>();

	Register<ParticleSystemEditor, ParticleSystem>();
	Register<ParticleSystemRendererEditor, ParticleSystemRenderer>();

	// アセット（AssetEditors.h）
	Register<TextureEditor, Texture>();
	Register<AudioClipEditor, AudioClip>();
	Register<MaterialEditor, Material>();
	Register<ShaderEditor, Shader>();
	Register<PhysicsMaterialEditor, PhysicsMaterial>();
	Register<VolumeProfileEditor, VolumeProfile>();
}


std::unique_ptr<Editor> ou::CustomEditor::Create(const Object &target) {
	static const auto isRegistered = (RegisterEngineEditors(), true);
	(void)isRegistered;

	const auto it = sFactories_.find(typeid(target));
	return it != sFactories_.end() ? it->second() : nullptr;
}
