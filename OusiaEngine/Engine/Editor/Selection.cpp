#include "Selection.h"
#include "Scene/GameObject.h"

using namespace ou;


void ou::Selection::SetActiveGameObject(GameObject *gameObject) {
	sActiveInstanceId_ = gameObject ? gameObject->GetInstanceId() : 0;
}


GameObject *ou::Selection::GetActiveGameObject() {
	auto *gameObject = dynamic_cast<GameObject *>(Object::FindObjectFromInstanceId(sActiveInstanceId_));
	return (gameObject && !gameObject->IsDestroyed()) ? gameObject : nullptr;
}
