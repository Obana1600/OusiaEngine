#include "GraphicsAllocators_DX12.h"
#include "Core/Fatal.h"
#include "Graphics/DirectX12/GraphicsDevice_DX12.h"

#include <bit>

using namespace ou;


void ou::Graphics::DescriptorAllocator::Initialize(ID3D12Device *d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorHeapBlock) {
	device = d3dDevice;
	desc.Type = type;
	desc.NumDescriptors = numDescriptorHeapBlock;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // シェーダ不可視
	descriptorSize = device->GetDescriptorHandleIncrementSize(type);
}


D3D12_CPU_DESCRIPTOR_HANDLE ou::Graphics::DescriptorAllocator::Allocate() {
	std::scoped_lock lock(locker);

	if (freeList.empty()) {
		BlockAllocate();
	}

	const auto handle = freeList.back();
	freeList.pop_back();
	return handle;
}


void ou::Graphics::DescriptorAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	std::scoped_lock lock(locker);
	freeList.push_back(handle);
}


void ou::Graphics::DescriptorAllocator::BlockAllocate() {
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &heap = heaps.emplace_back();
	auto hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	CheckHResult(hr, "ディスクリプタヒープの作成");

	const auto cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < desc.NumDescriptors; i++) {
		auto handle = cpuStart;
		handle.ptr += static_cast<SIZE_T>(i) * descriptorSize;
		freeList.push_back(handle);
	}
}


void ou::Graphics::AllocationHandler::Update(uint64_t currentFrameCount, uint32_t bufferCount) {
	std::scoped_lock lock(destroyLocker);
	frameCount = currentFrameCount;

	auto release = [&](auto &destroyer) {
		while (!destroyer.empty() && destroyer.front().second + bufferCount < frameCount) {
			destroyer.pop_front();
		}
	};
	release(destroyerResources);
	release(destroyerPipelines);

	// 番号は破棄せず、フリーリストに戻す
	while (!destroyBindlessRes.empty() && destroyBindlessRes.front().second + bufferCount < frameCount) {
		freeBindlessRes.push_back(destroyBindlessRes.front().first);
		destroyBindlessRes.pop_front();
	}
}


int ou::Graphics::AllocationHandler::AllocateResourceIndex() {
	std::scoped_lock lock(destroyLocker);

	// プールに空きがなければ -1 を返す
	if (freeBindlessRes.empty()) { return -1; }

	const auto index = freeBindlessRes.back();
	freeBindlessRes.pop_back();
	return index;
}




void ou::Graphics::CopyAllocator::Initialize(GraphicsDevice_DX12 *owner) {
	device = owner;
}


Graphics::CopyAllocator::CopyCMD ou::Graphics::CopyAllocator::Allocate(uint64_t stagingSize) {
	HRESULT hr{};
	CopyCMD cmd;

	// プールから大きさが足りるものを探す
	{
		std::scoped_lock lock(locker);
		for (size_t i = 0; i < freeList.size(); i++) {
			if (freeList[i].uploadBuffer.desc.size >= stagingSize) {
				cmd = std::move(freeList[i]);
				freeList.erase(freeList.begin() + i);
				break;
			}
		}
	}

	// なければ新規作成
	if (!cmd.IsValid()) {
		hr = device->GetNativeDevice()->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&cmd.allocator));
		CheckHResult(hr, "コピー用アロケータの作成");

		hr = device->GetNativeDevice()->CreateCommandList(
			0, D3D12_COMMAND_LIST_TYPE_COPY, cmd.allocator.Get(), nullptr, IID_PPV_ARGS(&cmd.list));
		CheckHResult(hr, "コピー用コマンドリストの作成");
		cmd.list->Close();

		hr = device->GetNativeDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&cmd.fence));
		CheckHResult(hr, "コピー用フェンスの作成");

		GpuBufferDesc uploadDesc{};
		uploadDesc.size = std::bit_ceil(stagingSize);
		uploadDesc.heapType = HeapType::UPLOAD;
		if (!device->CreateBuffer(&uploadDesc, nullptr, &cmd.uploadBuffer)) {
			FatalError("ステージングバッファの作成に失敗しました。");
		}
		device->SetName(&cmd.uploadBuffer, "CopyAllocator::uploadBuffer");
	}

	// 記録を始める
	hr = cmd.allocator->Reset();
	CheckHResult(hr, "コピー用アロケータのリセット");
	hr = cmd.list->Reset(cmd.allocator.Get(), nullptr);
	CheckHResult(hr, "コピー用コマンドリストのリセット");

	return cmd;
}


void ou::Graphics::CopyAllocator::Submit(CopyCMD cmd) {
	HRESULT hr = cmd.list->Close();
	CheckHResult(hr, "コピー用コマンドリストのクローズ");

	auto *queue = device->GetNativeCommandQueue(QUEUE_COPY);
	ID3D12CommandList *lists[] = {cmd.list.Get()};
	queue->ExecuteCommandLists(1, lists);

	// コピーが終わるまで CPU で待つ
	hr = queue->Signal(cmd.fence.Get(), 1);
	CheckHResult(hr, "コピー用フェンスのシグナル");
	if (cmd.fence->GetCompletedValue() < 1) {
		hr = cmd.fence->SetEventOnCompletion(1, nullptr);
		CheckHResult(hr, "コピー用フェンスの待機");
	}
	cmd.fence->Signal(0);

	// 再利用できるように返す
	std::scoped_lock lock(locker);
	freeList.push_back(std::move(cmd));
}