#include "ImmediateCommands.h"
#include "Graphics/GraphicsDevice.h"

using namespace ou;


Graphics::CommandList ou::ImmediateCommands::Acquire() {
	if (!sCmd_.IsValid()) {
		sCmd_ = Graphics::GetDevice()->BeginCommandList();
	}
	return sCmd_;
}


void ou::ImmediateCommands::OnSubmitted() {
	sCmd_ = {};
}
