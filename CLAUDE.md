# CG_DirectXGame

DirectX 12 の自作エンジン（Visual Studio）。リポジトリのルートには git と書式の設定（`.clang-format` / `.editorconfig`）だけを置き、ソースとソリューションは `CG_DirectXGame/` にある。ソリューションは `CG_DirectXGame/CG_DirectXGame.slnx` で、`Engine/`（静的ライブラリ）と `Game/`（exe）の 2 プロジェクト。以下のパスは `CG_DirectXGame/` からの相対パス。

- `Legacy/` は旧コードの置き場で、**ビルド対象外**。移行や作り直しの参考として読むだけにする
- シェーダは実行時に `DXCompiler` でコンパイルする。エンジン用は `Engine/EngineAssets/Shader/`（`FindEngineAssetsFolder()`）、ゲーム用は `Game/Assets/`（`FindAssetsFolder()`）から探す。どちらも exe の場所から親フォルダをたどって探すので、ビルド出力の `Generated/` は `CG_DirectXGame/` の中に置く
- ビルド設定は `Build/Common.props` に集約している（警告レベル 4・警告はエラー扱い）
- DirectXTex は `External/DirectXTex/DirectXTex_Desktop_2022_Win10.vcxproj` を `Engine` が参照し、`Engine.lib` にまとめてリンクされる。このプロジェクトは「クリーン」でコミット済みの `Shaders/Compiled/*.inc` を消さないように改変してある
- Debug / Develop のログは `Generated/Outputs/<構成>/Logs/`、Release のログは `%LOCALAPPDATA%/OusiaEngine/CG2_DirectXGame/Logs/`

## 描画層の作り直し（進行中）

描画層は Wicked Engine 方式の `GraphicsDevice` / `CommandList` に作り直している途中で、コードはユーザーが**写経**で書いている。
描画まわり（`Engine/Graphics/`、`Engine/Engine.*`、`Legacy/` からの移行、GameObject / Component、`GraphicsDevice` / `CommandList` / Wicked）を扱うときは、作業の前に必ず [docs/GraphicsDevice.md](docs/GraphicsDevice.md) を読み、そこに書かれた進め方に従う。
