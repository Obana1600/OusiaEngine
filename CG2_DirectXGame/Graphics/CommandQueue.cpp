#include "CommandQueue.h"
#include "Core/Log.h"
#include "Graphics/CommandAllocatorPool.h"

using namespace ou;


std::unique_ptr<CommandQueue> ou::CommandQueue::Create(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type) {
	std::unique_ptr<CommandQueue> q(new CommandQueue());

	// キューの生成
	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Type = type;
	auto hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&q->queue_));
	if (FAILED(hr)) {
		Log::Error("キューの作成に失敗しました。");
		return nullptr;
	}
	q->queue_->SetName(L"Queue");

	// フェンスの作成
	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&q->fence_));
	if (FAILED(hr)) {
		Log::Error("フェンスの作成に失敗しました。");
		return nullptr;
	}
	q->fence_->SetName(L"Queue Fence");

	// イベントの作成
	q->fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!q->fenceEvent_) {
		Log::Error("イベントの作成に失敗しました。");
		return nullptr;
	}

	// アロケータプールの作成
	q->allocatorPool_ = std::make_unique<CommandAllocatorPool>(device, type);

	return q;
}


ou::CommandQueue::~CommandQueue() {
	if (queue_ && fence_) {
		Flush();
	}

	if (fenceEvent_) {
		CloseHandle(fenceEvent_);
	}
}


void ou::CommandQueue::Execute(ID3D12CommandList *list) {
	ID3D12CommandList *lists[] = {list};
	queue_->ExecuteCommandLists(1, lists);
}


uint64_t ou::CommandQueue::Signal() {
	const auto value = nextFenceValue_++;
	queue_->Signal(fence_.Get(), value);
	return value;
}


bool ou::CommandQueue::IsFenceComplete(uint64_t fenceValue) const {
	return fence_->GetCompletedValue() >= fenceValue;
}


void ou::CommandQueue::WaitForFence(uint64_t fenceValue) {
	if (IsFenceComplete(fenceValue)) { return; }

	fence_->SetEventOnCompletion(fenceValue, fenceEvent_);
	WaitForSingleObject(fenceEvent_, INFINITE);
}


void ou::CommandQueue::Flush() {
	WaitForFence(Signal());
}


ID3D12CommandAllocator *ou::CommandQueue::Request() {
	return allocatorPool_->Request(fence_->GetCompletedValue());
}


void ou::CommandQueue::Discard(uint64_t fenceValue, ID3D12CommandAllocator *allocator) {
	allocatorPool_->Discard(fenceValue, allocator);
}


ID3D12CommandQueue *ou::CommandQueue::Queue() const {
	return queue_.Get();
}
