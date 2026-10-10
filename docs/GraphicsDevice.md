# GraphicsDevice 設計（Wicked Engine 方式）

描画層を Wicked Engine の `wi::graphics` と同じ形に作り直す計画。
D3D12 の呼び出しはすべて `GraphicsDevice` に集め、描画命令は `BeginCommandList()` が返す `CommandList` ハンドルを引数に渡して記録する。

参照元: https://github.com/turanszkij/WickedEngine の
`WickedEngine/wiGraphics.h`（型・desc）, `wiGraphicsDevice.h`（インターフェース）, `wiGraphicsDevice_DX12.h/.cpp`（実装）。

## 進め方: 写経

ユーザーが Wicked の構造を理解するために、コードは**ユーザー自身が写経する**。エージェントの役割は次のとおり。

1. 次に書くもの（1 ファイルまたは数関数ぶん）を決め、Wicked での対応箇所と「なぜそう作るか」をチャットで説明する。
2. 写経用のコードはチャットに示す。量は一度に 1 つの単位までにする。
3. ユーザーが書き終えたらファイルを読んでレビューする。バグ・Wicked との意味の違い・後のフェーズで困る点を指摘する。
4. フェーズの完了条件を満たしたら、下の「進捗」のチェックを更新する。

エージェントがファイルを編集するのは、このドキュメントの更新と、ユーザーが明示的に頼んだときだけ。

例外として、**検証用の `main.cpp`（テストコード）はエージェントが書く**（2026-10-10 にユーザーが依頼）。写経するのは `Engine/Graphics/` 以下などのエンジン本体のコードで、`main.cpp` はヘルパー関数（`CompileShader` / `LoadTexture` / `CreateSceneTargets` / `CreateStaticBuffer`）に分けて読みやすく保つ。書いたらビルドし、exe を数秒起動して、リポジトリのルートの `Generated/Outputs/Debug/Logs/` の最新ログに D3D12 のエラーがないことを確認する。

## 命名と正の情報源

- クラス名・関数名・引数の並び・desc のフィールド名は Wicked に合わせる（例: `GPUBufferDesc::bind_flags`、`BindConstantBuffer(const GPUBuffer*, uint32_t slot, CommandList cmd, uint64_t offset)`）。
- **ユーザーが書いたコードが Wicked と違っていたら、それはユーザーの好みなので、そのまま正とする**。Wicked に戻すよう提案はしない。以後の説明もユーザーのコードに合わせる。
- 名前空間・メンバ変数の `_` 接尾辞・`public: // メンバ関数` 形式のセクションコメント・日本語コメントといった既存のコーディングスタイルは、既存コードに従う。
- Wicked は時期によって API が変わっている。迷ったら upstream master に合わせる。

## 決定事項

| 項目 | 決定 |
|---|---|
| 抽象化 | Wicked と同じく `GraphicsDevice`（基底）＋ `GraphicsDevice_DX12`（実装）。実装は DX12 のみで、他の API は作らない前提 |
| アクセス | Wicked の `GetDevice()` と同じグローバルアクセサを使う。`DirectXCommon::GetInstance()` は置き換えて廃止する |
| リソース型 | `GraphicsDeviceChild { std::shared_ptr<void> internal_state; }` を基底にする。D3D12 の型はヘッダに出さず、`*_DX12` 内部構造体と `to_internal()` で `.cpp` 内に隠す |
| CommandList | 値型ハンドル（`void* internal_state`）。中身の `CommandList_DX12` はデバイスがプールして使い回す |
| 状態遷移 | 自動追跡はしない。`Barrier(const GPUBarrier*, uint32_t, CommandList)` で明示する。RenderPass はバックバッファの遷移を自分で行う |
| フレーム | `BUFFERCOUNT = 2`。`SubmitCommandLists()` が Close → Execute → Present → フレームフェンス → `BUFFERCOUNT` 前のフレームを待機 → 遅延破棄 → `frameCount++` までを一括で行う |
| ルートシグネチャ | **バインドレス方式**（2026-10-07 にユーザーが決定。CG2_DirectXGame リポジトリの `Engine/Graphics/RHI/RootSignature.cpp` がベース）。全 PSO 共通で、ルート CBV `b0`〜`b3` + 静的サンプラー `s0`〜`s3` + `CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED`。テクスチャはシェーダが `ResourceDescriptorHeap[index]` で引き、インデックスは CB で渡す。Wicked の `DescriptorBinder`（SRV テーブルへの flush）は作らない |
| シェーダモデル | バインドレスのため SM 6.6（`vs_6_6` / `ps_6_6`）。起動時に SM 6.6 と Resource Binding Tier 3 を確認する。コマンドリストでは `SetDescriptorHeaps` を `SetGraphicsRootSignature` より先に呼ぶ |
| アップロード | Wicked の `CopyAllocator` 相当。最初は「コピーして完了まで待つ」同期版でよい |
| 破棄 | Wicked の `AllocationHandler` 相当。フレーム番号付きの破棄キューに入れ、`BUFFERCOUNT` フレーム後に Release する |
| 後回しにするもの | サンプラーのバインドレス化（Phase 4 の後。空きインデックス管理を流用できる）、Compute/Copy キューの非同期化、マルチスレッド記録、Vulkan |

## フォルダ構成（2026-10-10 に CG2_DirectXGame リポジトリと同じ形に整理）

リポジトリのルート（`C:\CG_DirectXGame`）には `.git` / `.gitignore` / `.clang-format` / `.editorconfig` / `CLAUDE.md` / `docs/` だけを置き、ソリューションとソースは `OusiaEngine/` に入れる（ユーザーの希望）。以下のパスは `OusiaEngine/` からの相対パス。

- `OusiaEngine.slnx` … `Engine` / `Game` / `External/DirectXTex` の 3 プロジェクト
- `Engine/`（`Engine.vcxproj`、静的ライブラリ）… `Engine.h/.cpp`、`Core/`、`Graphics/`（`GraphicsTypes` / `GraphicsDevice` / `GraphicsDevice_DX12` / `DXCompiler`）、`Math/`、`Input/`、`Platform/`（`Window`）、`EngineAssets/Shader/`（`Blit.*`）
- `Game/`（`Game.vcxproj`、exe）… `main.cpp`、`GameScene`、`Assets/`（旧 `resources/`）、`UserSettings/`
- `Legacy/`（**ビルド対象外**）… `2d/`（ImGuiManager / Sprite）、`3d/`、`base/`（DirectXCommon / TextureManager / CreateBufferResource）、`Graphics/`（旧 RHI クラス）、`shader/`（Sprite / Model の旧シェーダと、Phase 3・4 の検証用 Triangle / Textured）
- `External/` … DirectXTex（`Engine` が参照し、`Engine.lib` にまとめてリンクされる）、imgui（`Engine.vcxproj` でコンパイル）、spdlog
- `Generated/` … ビルド出力。ユーザーの希望で、リポジトリのルート（`.git` と同じ層）に置く。出力先は `Build/Common.props` だけで決める。エディタ版はアセットを `SOURCE_DIR`（ビルド時に埋め込む `OusiaEngine/` の絶対パス）から探すので、出力先の場所に依存しない
- `Build/Common.props` … 全プロジェクト共通の設定。`#include` は `Engine/` からの相対パス（例: `"Graphics/GraphicsDevice.h"`）、外部ライブラリは `External/` からの相対パス（例: `"imgui/imgui.h"`）
- `DXCompiler::CompileShader` はフルパスを受け取る。`Engine.cpp` は `FindEngineAssetsFolder() / "Shader" / ファイル名` を渡す

## 新しいファイル（`Engine/Graphics/`）

- `GraphicsTypes.h` … Wicked の `wiGraphics.h` 相当。enum（`Format`, `Usage`, `BindFlag`, `ResourceMiscFlag`, `ShaderStage`, `ResourceState`, `PrimitiveTopology`, `QUEUE_TYPE` …）、desc（`GPUBufferDesc`, `TextureDesc`, `SamplerDesc`, `SwapChainDesc`, `PipelineStateDesc`, `InputLayout`, `RasterizerState`, `BlendState`, `DepthStencilState`）、`Viewport` / `Rect` / `SubresourceData` / `RenderPassImage` / `GPUBarrier`、リソース型（`GPUResource`, `GPUBuffer`, `Texture`, `Shader`, `Sampler`, `PipelineState`, `SwapChain`）、`CommandList`
- `GraphicsDevice.h` … `wiGraphicsDevice.h` 相当。純粋仮想インターフェースと `GetDevice()`
- `GraphicsDevice_DX12.h/.cpp` … 実装。内部構造体（`Resource_DX12`, `Texture_DX12`, `SwapChain_DX12`, `PipelineState_DX12`, `CommandList_DX12`, `CopyAllocator`, `AllocationHandler`, バインドレス用のディスクリプタヒープ）は `.cpp` 側に置く

## 置き換えて消すもの

2026-10-10 のフォルダ整理で、旧クラスはすべて `Legacy/` に移してビルド対象から外した（`DirectXCommon`、`D3D12Device`、`CommandQueue`、`SwapChainOld`、`GpuResourceOld`、`GraphicsConfig.h`、`CommandContext`、`CommandAllocatorPool`、`CreateBufferResource.h`、`TextureManager`、`2d/`、`3d/`）。消さずに残しているのは、移行や作り直しのときに参考にするため。

まだ移行していない `ImGuiManager` / `TextureManager` / `Sprite` は、Phase 5 の単位 2〜4 で `Legacy/` を参考にしながら `Engine/` に作り直す。

### 動作確認

`Game/main.cpp` は `Engine` と `GameScene` を使う本来の形に戻っている。検証したいものは `GameScene` に書くか、必要ならエージェントが `main.cpp` を一時的に書き換える。

## フェーズ

各フェーズの最後の行が完了条件。完了条件は、ビルドが通り、実行して目で確認できること。

### Phase 1: 型とインターフェース
`GraphicsTypes.h` と `GraphicsDevice.h` を作る。`GraphicsDevice_DX12` は全メソッドを空実装にした状態でよい。
**完了条件**: 既存の描画を壊さずにビルドが通る。

### Phase 2: デバイス・CommandList・フレームループ → 画面クリア
- コンストラクタ: factory / adapter / device / graphics キュー / `frame_fence[BUFFERCOUNT][QUEUE_COUNT]`
- `CommandList_DX12`: `commandAllocators[BUFFERCOUNT]` とコマンドリストを 1 つずつ持つ。`BeginCommandList(QUEUE_GRAPHICS)` は、プールから取り出して Reset する
- `SubmitCommandLists()`、`WaitForGPU()`、`GetFrameCount()`、`GetBufferIndex()`
- `CreateSwapChain(const SwapChainDesc*, HWND, SwapChain*)`、`RenderPassBegin(const SwapChain*, CommandList)`、`RenderPassEnd(CommandList)`
- `main.cpp` を検証用ループに置き換える
**完了条件**: `SwapChainDesc::clearColor` の色で画面が塗られ、ウィンドウのリサイズと終了時に D3D12 のエラーが出ない。

### Phase 3: Buffer・Shader・PSO → 三角形
- `CreateBuffer(const GPUBufferDesc*, const void* initial_data, GPUBuffer*)`（初期データは `CopyAllocator` で転送する）
- `CreateShader(ShaderStage, const void* shadercode, size_t size, Shader*)`
- 固定ルートシグネチャ、`CreatePipelineState(const PipelineStateDesc*, PipelineState*)`
- `BindPipelineState` / `BindVertexBuffers` / `BindIndexBuffer` / `BindViewports` / `BindScissorRects` / `Draw` / `DrawIndexed`
**完了条件**: 頂点カラー付きの三角形が表示される。

### Phase 4: 定数バッファ・テクスチャ・深度 → テクスチャ付きの回転する Quad
- `BindConstantBuffer`、フレームごとのリング確保 `AllocateGPU(uint64_t size, CommandList)` → `GPUAllocation`、`UpdateBuffer`
- CPU 側のディスクリプタヒープ（RTV / DSV）のアロケータ。`SwapChain_DX12` の自前 RTV ヒープもこれに置き換える
- バインドレスヒープ（Phase 3 で作成済み）の空きインデックス管理と、`GetDescriptorIndex(const GpuResource*, ViewType)`（Wicked と同名）
- `CreateTexture(const TextureDesc*, const SubresourceData*, Texture*)`（SRV をバインドレスヒープに作る）、`Barrier`
- `BindResource` / `BindSampler` / `CreateSampler` は、`GraphicsDevice.h` / `GraphicsDevice_DX12.h` の宣言を TODO 付きでコメントアウトし、`.cpp` の空実装も外す（2026-10-09 にユーザーが決定）。`CreateSampler` は、サンプラーのバインドレス化（`SAMPLER_HEAP_DIRECTLY_INDEXED` + シェーダから見える SAMPLER ヒープ + `GetDescriptorIndex(const Sampler*)`）をするときに復活させる。それまでサンプラーは静的サンプラー `s0`〜`s3` だけで足りる
- `RenderPassBegin(const RenderPassImage*, uint32_t count, CommandList)` で深度付きの描画
**完了条件**: uvChecker.png を貼った Quad が、毎フレーム CB を更新して回転し、深度テストも効いている。

### Phase 5: 既存コードの移行
1. ImGui: `GetNativeDevice()` と `GetNativeCommandList(CommandList)` の逃げ道を用意し、imgui_impl_dx12 をそのまま使う
2. `TextureManager` → `Sprite` を移行する。`3d/` のクラスは移行せず `Legacy/` に退避し、Phase 6 でコンポーネントとして作り直す
3. `DirectXCommon` と旧クラス（`D3D12Device` / `CommandQueue` / `SwapChainOld` / `GpuResourceOld` / `GraphicsConfig.h`）を削除し、`main.cpp` を `Engine` / `GameScene` を使う形に戻す
**完了条件**: `Engine` + `GameScene` で Sprite と ImGui が描画され、ビルド対象のコードに `DirectXCommon` と旧クラスへの参照が残っていない。

## 進捗

- [x] Phase 1: 型とインターフェース
- [x] Phase 2: 画面クリア
- [x] Phase 3: 三角形
- [x] Phase 4: テクスチャ付きの回転する Quad
- [ ] Phase 5: 既存コードの移行・DirectXCommon の削除
- [ ] Phase 6: Unity 風の GameObject / Component

<!-- 作業中のメモ（どこまで写経したか・保留中の判断）はこの下に追記する -->

### Wicked からの名前変更（ユーザーの好み。以後こちらを使う）

| Wicked | このプロジェクト |
|---|---|
| `wi::graphics` | `ou` |
| `QUEUE_TYPE` | `QueueType` |
| `Usage` | `HeapType` |
| `SubresourceType` | `ViewType` |
| `enable_bitmask_operators` / `has_flag` | `EnableBitmaskOperators` / `HasFlag` |
| desc・構造体のフィールド（snake_case） | camelCase（`bind_flags` → `bindFlags`、`usage` → `heapType`、`internal_state` → `internalState`） |
| `GPU` で始まる型（`GPUBufferDesc` / `GPUResource` / `GPUBuffer` / `GPUBarrier`） | `Gpu`（`GpuBufferDesc` / `GpuResource` / `GpuBuffer` / `GpuBarrier`） |
| `Rect` | `Scissor` |
| `ValidationMode::GPU` | `ValidationMode::Gpu` |
| `GPUResource::Type::UNKNOWN_TYPE` | `UNKNOWN` |
| `BUFFERCOUNT` / `WaitForGPU` | `BUFFER_COUNT` / `WaitForGpu` |
| `Create*` は `const` | `const` なし |
| `SetName(..., const char*)` | `SetName(..., std::string_view)` |
| `.cpp` の `dx12_internal` 名前空間 | 使わない。`*_DX12` 構造体・`ConvertFormat` などは `.cpp` の `namespace ou` に直接置く |
| `to_internal` / `_ConvertFormat` | `ToInternal` / `ConvertFormat`（SRGB → 非 SRGB は `ConvertFormatNonSrgb`） |
| `CommandList_DX12::commandAllocators` / `commandLists`、`CopyCMD::commandAllocator` / `commandList` | `allocators` / `lists`、`allocator` / `list` |
| `init()` など内部型の初期化関数 | `Initialize()` |

上の表にない構造体名・クラス名・関数名は Wicked のまま使う。

### 保留中の判断・決まったこと

- 既存の `ou::SwapChain` は新しく作る `SwapChain` 型と衝突するため、`SwapChainOld` にリネームした。旧 `GpuResource` も同じ理由で `GpuResourceOld` にリネームした。どちらも Phase 5 で削除する。
- 消える予定の既存クラスはリネームしない。移行後も残るクラス（`Sprite` / `Model` など）の API のリネームは、Phase 5 の書き換えと同時か、その後に行う。

### Phase 1 の写経状況

- [x] `GraphicsTypes.h` の enum
- [x] desc 構造体
- [x] リソース型・`CommandList`・`Viewport` / `Scissor` / `GpuBarrier` / `RenderPassImage`
- [x] `GraphicsDevice.h`
- [x] `GraphicsDevice_DX12` の空実装

### Phase 2 の写経状況

- [x] 1. コンストラクタ（`D3D12Device` から移植、`ValidationMode`、キュー × 3、フレームフェンス）。デストラクタで `WaitForGpu()` を呼ぶ処理は単位 3 で追加する
- [x] 2. `CommandList_DX12` と `BeginCommandList`
- [x] 3. `SubmitCommandLists` / `WaitForGpu` / 遅延破棄（`AllocationHandler`）。Present は単位 5 で `SubmitCommandLists` の Signal の直後に入れる
- [x] 4. `SwapChain_DX12` と `CreateSwapChain`（`SwapChainOld` から移植）。RTV ヒープは `SwapChain_DX12` が自前で持つ仮実装で、Phase 4 でディスクリプタアロケータに置き換える
- [x] 5. `RenderPassBegin` / `RenderPassEnd`（スワップチェーン版）と、`main.cpp` の検証ループ。画面クリア・リサイズの動作は確認済み

### Phase 3 の写経状況

- [x] 1. `Resource_DX12` と `CreateBuffer`（UPLOAD / DEFAULT、初期データなし）、`SetName`
- [x] 2. `CopyAllocator`（同期版。Submit で CPU が完了を待つ）と、DEFAULT バッファへの初期データ転送
- [x] 3. `Shader_DX12` と `CreateShader`。検証用シェーダは `resources/shader/Triangle.*`（今は `Legacy/shader/`）
- [x] 4. バインドレスのルートシグネチャ、SM 6.6 の確認、バインドレスヒープの作成と `BeginCommandList` でのセット
- [x] 5. `PipelineState_DX12` と `CreatePipelineState`（デフォルトの state を含む）。Wicked と同じ `RenderPassInfo` を追加し、`CreatePipelineState` の第 3 引数で必須にする（Wicked の「描画時に遅延作成」は後回し）
- [x] 6. `BindViewports` / `BindScissorRects` / `BindPipelineState` / `BindVertexBuffers` / `BindIndexBuffer` / `Draw` / `DrawIndexed` と、三角形の検証ループ。`BindConstantBuffer`（ルート CBV）もここで実装済み

### Phase 5 の方針（2026-10-10 にユーザーが決定）

- **定数バッファは描画時に毎回確保する**：`WorldTransform` / `Camera` / `Material` / `DirectionLight` は行列などのデータだけを持ち、`constBuffer_` と `TransferMatrix` をなくす。描画のたびに `BindDynamicConstantBuffer` で渡す（Wicked と同じ）
- **`CommandList` は引数で渡す**：`GameScene::Draw(CommandList cmd)` から `Sprite` / `Model` の描画関数へ流す
- **Phase 5 も写経で進める**（`main.cpp` だけはエージェントが書く）
- フレームの流れは `Engine` が持つ：`BeginDraw()` がシーン用 RT + 深度の RenderPass を開始してコマンドリストを返し、`EndDraw(cmd)` が Blit → ImGui → Submit → 固定 FPS の待機までを行う
- ImGui は imgui_impl_dx12 をそのまま使う。`GraphicsDevice_DX12` に D3D12 を直接返す逃げ道（`GetNativeDevice` / `GetNativeQueue` / `GetNativeCommandList`）を足し、ImGui 専用の小さな SRV ヒープを `ImGuiManager` が持つ。ImGui はヒープとルートシグネチャを差し替えるので、スワップチェーンのパスの**最後**に描く
- シェーダは `t0` / `s0` のテーブル方式から、バインドレスの番号を CB で渡す方式に書き換える

### Phase 5 の写経状況

- [x] 1. `Engine` を新しいデバイスで組み直す（ログ・デバイス・スワップチェーン・シーン用 RT・Blit・固定 FPS）、`GameScene::Draw(cmd)`、`main.cpp`。シーン用 RenderPass の構成は `Engine::GetRenderPassInfo()`。スワップチェーンとシーン用 RT は `B8G8R8A8_UNORM_SRGB`。Debug / Release とも起動・終了を確認済み
- [ ] 2. `ImGuiManager`（`Legacy/2d/` を参考に `Engine/` に作り直す。`GraphicsDevice_DX12` の `GetNativeDevice` / `GetNativeQueue` / `GetNativeCommandList` はまだ追加されていない）
- [ ] 3. `TextureManager`（`Texture` とバインドレスの番号を返す。DirectXTex はフォルダ整理のときに `Engine` から使えるようにしてある）
- [ ] 4. `Sprite`
- [x] 5. 旧クラスを `Legacy/` に移してビルド対象から外す（2026-10-10 のフォルダ整理で完了。削除ではなく退避）

`3d/` のクラスは移行しない。Phase 6 で Unity 風のコンポーネントとして作り直す（2026-10-10 にユーザーが決定）。

## Phase 6: Unity 風の GameObject / Component

ユーザーの希望で、Transform や Model を Unity のようなコンポーネントにする。Wicked の ECS（`wi::ecs` / `wi::scene`）ではなく、**Unity 風の GameObject 方式**を採る（2026-10-10 にユーザーが決定）。そのためこのフェーズは Wicked の写経ではなく、設計から一緒に決める。

叩き台（詳細は Phase 6 の開始時にユーザーと決める）:

- `GameObject`：名前と Component のリストを持つ。`AddComponent<T>()` / `GetComponent<T>()`。`Transform` は必ず 1 つ持つ
- `Component`：基底クラス。持ち主の `GameObject` への参照と、仮想関数の `Start` / `Update` / `OnImGui` など
- `Transform`：位置・回転（`Quaternion`）・スケール・親子関係・ワールド行列
- `Camera`・`MeshRenderer`（`Mesh` と `Material` を参照する）・`DirectionalLight` などの Component
- `Scene`：`GameObject` を持ち、更新と描画を回す。描画では Camera / Light / MeshRenderer を集め、`BindDynamicConstantBuffer` でルート CBV `b0`〜`b3` に渡す（CG2 リポジトリの `Common.hlsli` の PerDraw / PerMaterial / PerCamera / PerFrame の割り当てが参考になる）
- 旧クラスの OBJ 読み込みなどは `Legacy/` から参考にする

### Phase 4 の写経状況

- [x] 1. CPU 側のディスクリプタアロケータ（RTV / DSV）。`SwapChain_DX12` の自前 RTV ヒープを置き換える
- [x] 2. バインドレスヒープの空きインデックス管理と `GetDescriptorIndex`。番号の返却は `BUFFER_COUNT` フレーム遅らせる
- [x] 3. `Texture_DX12` と `CreateTexture`（初期データは `CopyAllocator` で転送、SRV / RTV / DSV を作る）と `Barrier`。初期データ付きのテクスチャは COMMON で作り、`desc.layout` への遷移を `initTransitions_` に予約して、次の `BeginCommandList` で実行する。`main.cpp` は `CoInitializeEx` と `LogSystem` を自前で用意している
- [x] 4. `RenderPassBegin(const RenderPassImage*, ...)`。オフスクリーンの RT（`SceneColor`, R8G8B8A8_UNORM_SRGB）+ 深度（`SceneDepth`, D32_FLOAT）に描き、`Blit.*.hlsl`（SV_VertexID の全画面三角形 + `ResourceDescriptorHeap`）でスワップチェーンに合成する。`RenderPassInfo::From(images, count)` も追加済み
- [x] 5. `AllocateGpu`（Wicked の `AllocateGPU`。コマンドリストごと・バッファ番号ごとの線形アロケータ）と `BindDynamicConstantBuffer`。`AllocateGpu` / `GpuAllocation` / `GpuLinearAllocator` は基底の `GraphicsDevice.h` にあり、DX12 は `GetFrameAllocator` だけを実装する。`AlignTo` は `GraphicsTypes.h` に移動済み
- [x] 6. uvChecker.png を貼った回転する Quad の検証ループ（ユーザーが目視で確認済み）
