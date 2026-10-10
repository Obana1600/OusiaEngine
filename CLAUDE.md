# CG_DirectXGame

DirectX 12 の自作エンジン（Visual Studio）。リポジトリのルートには git と書式の設定（`.clang-format` / `.editorconfig`）だけを置き、ソースとソリューションは `CG_DirectXGame/` にある。ソリューションは `CG_DirectXGame/CG_DirectXGame.slnx` で、`Engine/`（静的ライブラリ）と `Game/`（exe）の 2 プロジェクト。以下のパスは `CG_DirectXGame/` からの相対パス。

- `Legacy/` は旧コードの置き場で、**ビルド対象外**。移行や作り直しの参考として読むだけにする
- シェーダは実行時に `DXCompiler` でコンパイルする。エンジン用は `Engine/EngineAssets/Shader/`（`FindEngineAssetsFolder()`）、ゲーム用は `Game/Assets/`（`FindAssetsFolder()`）から探す。エディタ版（Debug / Develop、`WITH_EDITOR`）は、`Build/Common.props` がビルド時に埋め込む `SOURCE_DIR`（`CG_DirectXGame/` の絶対パス）を基準にする。Release は exe の隣を見る（ビルド後にコピーされる）
- ビルド設定と出力先は `Build/Common.props` に集約している（警告レベル 4・警告はエラー扱い）。ビルド出力の `Generated/` はリポジトリのルート（`.git` と同じ層）に置く。各 vcxproj に出力先を個別に書かない
- `Engine.vcxproj.filters` / `Game.vcxproj.filters` のフィルタは VS 既定の「ソース ファイル」「ヘッダー ファイル」「リソース ファイル」の 3 つだけ（ユーザーの希望）。フォルダごとのフィルタは作らない。ファイルを vcxproj に追加するときは、filters にも拡張子に応じて `.cpp` → ソース、`.h` → ヘッダー、`.hlsl` / `.hlsli` → リソースで追加する
- `Common.props` のパスのマクロは 2 つある：`$(SourceRoot)` は `CG_DirectXGame/`（Engine / Game / External の場所）、`$(RepoRoot)` はリポジトリのルート
- DirectXTex は `External/DirectXTex/DirectXTex_Desktop_2022_Win10.vcxproj` を `Engine` が参照し、`Engine.lib` にまとめてリンクされる。このプロジェクトは「クリーン」でコミット済みの `Shaders/Compiled/*.inc` を消さないように改変してある
- Debug / Develop のログは、リポジトリのルートの `Generated/Outputs/<構成>/Logs/`、Release のログは `%LOCALAPPDATA%/OusiaEngine/CG2_DirectXGame/Logs/`

## 描画層の作り直し（進行中）

描画層は Wicked Engine 方式の `GraphicsDevice` / `CommandList` に作り直している途中で、コードはユーザーが**写経**で書いている。
描画まわり（`Engine/Graphics/`、`Engine/Engine.*`、`Legacy/` からの移行、GameObject / Component、`GraphicsDevice` / `CommandList` / Wicked）を扱うときは、作業の前に必ず [docs/GraphicsDevice.md](docs/GraphicsDevice.md) を読み、そこに書かれた進め方に従う。
