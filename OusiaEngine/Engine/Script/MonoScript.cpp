#include "MonoScript.h"
#include "Script/MonoBehavior.h"

using namespace ou;


MonoScript *ou::MonoScript::FromMonoBehavior(MonoBehavior *behavior) {
	return behavior ? behavior->script_.get() : nullptr;
}
