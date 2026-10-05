#pragma once

#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <wrl/client.h>


namespace ou {

class CommandQueue {
public: // 静的ファクトリ

	static std::unique_ptr<CommandQueue> Create(
		ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type);


public: // メンバ関数

	// デストラクタ
	~CommandQueue();

	// コマンドリストを GPU に送る
	void Execute(ID3D12CommandList *list);

	// フェンスを進め、その値を返す
	uint64_t Signal();

	// GPU がフェンス値に到達したか
	bool IsFenceComplete(uint64_t fenceValue) const;

	// GPU がフェンス値に到達するまで CPU を止める
	void WaitForFence(uint64_t fenceValue);

	// 送った命令がすべて終わるまで待つ
	void Flush();

	ID3D12CommandQueue *Queue() const;


private: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	HANDLE fenceEvent_ = nullptr;

	uint64_t nextFenceValue_ = 1;


private: // コンストラクタ

	CommandQueue() = default;

	CommandQueue(const CommandQueue &) = delete;
	CommandQueue &operator=(const CommandQueue &) = delete;
};

} // namespace ou
