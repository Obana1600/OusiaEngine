#include "EngineTypes.h"
#include "Animation/Skeletal/SkeletalAnimator.h"
#include "Audio/AudioListener.h"
#include "Audio/AudioSource.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemRenderer.h"
#include "Physics/BoxCollider.h"
#include "Physics/CapsuleCollider.h"
#include "Physics/CharacterController.h"
#include "Physics/Rigidbody.h"
#include "Physics/SphereCollider.h"
#include "Render/Camera.h"
#include "Render/Light.h"
#include "Render/LineRenderer.h"
#include "Render/MeshRenderer.h"
#include "Render/SkinnedMeshRenderer.h"
#include "Scene/Transform.h"
#include "Script/MonoBehavior.h"
#include "Serialize/TypeRegistry.h"
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


void ou::RegisterEngineTypes() {
	TypeRegistry::Register<Transform>("Transform");

	TypeRegistry::Register<Camera>("Camera");
	TypeRegistry::Register<Light>("Light");
	TypeRegistry::Register<MeshRenderer>("MeshRenderer");
	TypeRegistry::Register<SkinnedMeshRenderer>("SkinnedMeshRenderer");
	TypeRegistry::Register<SkeletalAnimator>("SkeletalAnimator");

	TypeRegistry::Register<BoxCollider>("BoxCollider");
	TypeRegistry::Register<SphereCollider>("SphereCollider");
	TypeRegistry::Register<CapsuleCollider>("CapsuleCollider");
	TypeRegistry::Register<CharacterController>("CharacterController");
	TypeRegistry::Register<Rigidbody>("Rigidbody");

	TypeRegistry::Register<AudioSource>("AudioSource");
	TypeRegistry::Register<AudioListener>("AudioListener");

	TypeRegistry::Register<RectTransform>("RectTransform");
	TypeRegistry::Register<Canvas>("Canvas");
	TypeRegistry::Register<CanvasRenderer>("CanvasRenderer");
	TypeRegistry::Register<GraphicRaycaster>("GraphicRaycaster");
	TypeRegistry::Register<Image>("Image");
	TypeRegistry::Register<Text>("Text");
	TypeRegistry::Register<Button>("Button");
	TypeRegistry::Register<EventSystem>("EventSystem");
	TypeRegistry::Register<StandaloneInputModule>("StandaloneInputModule");

	TypeRegistry::Register<ParticleSystem>("ParticleSystem");
	TypeRegistry::Register<ParticleSystemRenderer>("ParticleSystemRenderer");
	TypeRegistry::Register<LineRenderer>("LineRenderer");

	TypeRegistry::Register<MonoBehavior>("MonoBehavior");
}
