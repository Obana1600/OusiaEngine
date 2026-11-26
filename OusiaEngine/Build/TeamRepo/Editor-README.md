# OusiaEngine エディターガイド

配布版 OusiaEditor で、シーンを編集し、AngelScript で動きを付け、Windows 向けのゲームを書き出すためのガイドです。エディターのバージョンは同じフォルダーの `version.txt` で確認できます。このガイドはエディターの更新時に差し替わります。

ガイドは 4 つあります。

| ガイド | 内容 |
|---|---|
| **エディターガイド**（このファイル） | エディターの操作、アセットとチーム作業、ポストプロセスの設定、ゲームの書き出し |
| [スクリプトの書き方](Scripting.md) | AngelScript で動きを付ける。最初のスクリプト、MonoBehavior、コールバック |
| [スクリプト API リファレンス](Script-API.md) | スクリプトから使える型と関数の一覧 |
| [シェーダとポストエフェクトの書き方](Shaders.md) | マテリアルとシェーダ、Volume の効果、Full Screen Pass |

このファイルの目次：

- [最初のシーンを作る](#最初のシーンを作る)
- [エディターの操作](#エディターの操作)
- [アセットとチーム作業](#アセットとチーム作業)
- [ポストプロセス](#ポストプロセス)
- [ゲームを書き出す](#ゲームを書き出す)
- [困ったとき](#困ったとき)

## 最初のシーンを作る

1. チームのリポジトリをクローンし、Windows で `Editor/OusiaEditor.exe` を起動します。配布版は C++ のビルドなしで使えます。DirectX 12、Shader Model 6.6、Resource Binding Tier 3 に対応する GPU とドライバーが必要です。
2. **File > New Scene** を選びます。新しいシーンには **Main Camera** と **Directional Light** が入っています。
3. **GameObject > 3D Object > Cube** を選びます。**Hierarchy** の Cube を選ぶと、**Inspector** で名前と Transform を編集できます。
4. **Ctrl+S** で `Assets/Scenes/FirstScene.scene` に保存します。
5. 上部の **Play** ボタン（**Ctrl+P**）を押し、**Game** でカメラから見た Cube を確認します。もう一度押すと編集に戻ります。

Play 中のシーンの変更は、停止すると Play 前の状態に戻ります。残したい位置や設定は、停止後に編集して保存します。

## エディターの操作

閉じたパネルは **Window** メニューから開けます。

| パネル | 操作 |
|---|---|
| **Hierarchy** | シーン内の GameObject を選ぶ。別の GameObject の行の中央へドラッグすると、その子にできる。背景へドラッグすると、ルートへ戻せる。 |
| **Scene** | 編集用の視点で配置を見る。GameObject をクリックして選び、ギズモで移動・回転・拡縮する。**Local / Global** でギズモの軸を切り替える。 |
| **Inspector** | 選んだ GameObject の Transform やコンポーネントの値を編集する。**Add Component** でコンポーネントやスクリプトを付ける。 |
| **Game** | ゲームのカメラから見える画面。Play 中にここをクリックすると、ゲームに入力を渡せる。 |
| **Project** | `Assets/` とエンジン付属のアセットを参照する。`.scene` をダブルクリックすると開く。 |
| **Console** | ログ、警告、エラーを見る。ファイル位置があるエントリは、ダブルクリックで外部エディターから開ける。**Collapse** で同じログをまとめられる。 |

| 操作 | キー・マウス |
|---|---|
| Scene の見回しと移動 | 右ドラッグで見回す。右ボタンを押したまま **W/A/S/D** で前後左右、**Q/E** で下降・上昇、**Shift** で加速 |
| Scene の視点調整 | 中ドラッグで平行移動、**Alt+左ドラッグ**で回り込み、ホイールでズーム、**F** で選択物に寄る |
| Scene のツール切り替え | **Q** 視点、**W** 移動、**E** 回転、**R** 拡縮 |
| シーンを保存・別名保存 | **Ctrl+S** / **Ctrl+Shift+S** |
| 編集を取り消す・やり直す | **Ctrl+Z** / **Ctrl+Y** |
| GameObject を複製・名前変更・削除 | **Ctrl+D** / **F2** / **Delete** |
| 再生・一時停止・1 フレーム進める | **Ctrl+P** / **Ctrl+Shift+P** / **Ctrl+Alt+P** |
| Console を開く | **Ctrl+Shift+C** |

**GameObject** メニュー（Hierarchy の右クリックでも同じ）で作れるもの：

| 項目 | 作られるもの |
|---|---|
| **Create Empty** | Transform だけの空の GameObject |
| **3D Object** | Cube・Sphere・Capsule・Cylinder・Plane・Quad（Quad 以外は形に合った Collider 付き） |
| **Camera** | Camera |
| **Light** | Directional Light・Point Light・Spot Light |
| **UI** | Canvas・Text・Image・Button・Event System（Canvas と Event System がなければ一緒に作る） |

Rigidbody・AudioSource・ParticleSystem・Volume などのコンポーネントは、Inspector の **Add Component** から付けます。

Project の右クリック > **Create** で作れるもの：

| 項目 | 作られるもの | 説明 |
|---|---|---|
| **Folder** | フォルダー | |
| **MonoBehavior Script** | `.as` | [スクリプトの書き方](Scripting.md) |
| **Material** | `.mat` | [シェーダとポストエフェクトの書き方](Shaders.md#マテリアル) |
| **Unlit Shader** | `.shader` と HLSL 2 つ | [シェーダとポストエフェクトの書き方](Shaders.md#シェーダを作る) |
| **Physics Material** | `.physicsmaterial` | Collider の摩擦と跳ね返り |
| **Volume Profile** | `.volumeprofile` | [ポストプロセス](#ポストプロセス) |

**Edit > Project Settings...** で編集できるもの：

| 項目 | 内容 |
|---|---|
| **Graphics** | 全体のポストプロセス（Default Volume Profile）と、Full Screen Pass（Renderer Features） |
| **Physics** | 物理の設定 |
| **Player** | 製品名とウィンドウ |
| **Tags and Layers** | タグとレイヤー |
| **Time** | 時間の設定 |

## アセットとチーム作業

素材ファイルはエクスプローラーから `Assets/` 以下へコピーします。外部で追加した素材をすぐ使えないときは、エディターを開き直すと起動時に読み込まれます。

| 素材 | 主な対応形式 | 使い方 |
|---|---|---|
| モデル | `.obj`、`.gltf`、`.glb` | Project から Scene または Hierarchy へドラッグする。外部データを参照するモデルは、テクスチャや `.bin` も相対位置を保ってコピーする。 |
| 画像 | `.png`、`.jpg`、`.jpeg` | Inspector で画像を受け取る欄にドラッグする。 |
| 音声 | `.wav`、`.mp3` | AudioSource の Clip に割り当てる。 |
| フォント | `.ttf`、`.otf` | UI の Text に割り当てる。 |
| マテリアル・物理マテリアル | `.mat`、`.physicsmaterial` | 対応するコンポーネントの参照欄に割り当てる。 |
| シェーダ | `.shader`、`.hlsl`、`.hlsli` | マテリアルのシェーダの欄に `.shader` を割り当てる。書き換えたらエディターを開き直す。 |
| Volume Profile | `.volumeprofile` | Volume の `sharedProfile` や Project Settings の Graphics に割り当てる。 |
| シーン・Prefab・スクリプト | `.scene`、`.prefab`、`.as` | シーンの保存、Prefab の作成、スクリプトの作成で生成する。 |

組み立てた GameObject を使い回すには、Hierarchy から Project のフォルダーへドラッグして **Prefab** を作ります。配置するときは、Project の Prefab を Scene または Hierarchy へドラッグします。スクリプトから複製する場合は、Prefab を `GameObject@` のフィールドに割り当てて `Instantiate` に渡します。

| 場所 | チームでの扱い |
|---|---|
| `Assets/` | 素材、シーン、Prefab、スクリプトを共有する。アセットと一緒に **`.meta` もコミット**する。参照に使う GUID が入っているため、移動や名前変更には Project の操作を使う。 |
| `ProjectSettings/` | Build Settings、製品名、タグ、レイヤーなど、チーム共通の設定を共有する。 |
| `Editor/` | ビルド済みのエディター一式。担当者が更新する。更新を pull する前にエディターを閉じる。 |
| `UserSettings/`、`as.predefined`、`Editor/Logs/` | PC ごとに生成される。配布テンプレートの `.gitignore` で除外される。 |

同じ `.scene` を複数人で同時に編集すると、Git の競合を解消しづらくなります。シーンごとに担当を決め、別々に作れる部品は Prefab にします。

## ポストプロセス

画面全体の色や明るさ（Bloom・トーンマップ・色調・周辺減光）は Volume で設定します。手順の概略：

1. Project の **Create > Volume Profile** で Profile を作る
2. Inspector の **Add Override** で効果を足し、変えたい値のチェックを入れて値を変える
3. GameObject に **Volume** を付けて Profile を入れる。すべてのシーンにかけるものは **Project Settings > Graphics > Default Volume Profile** に入れる

カメラの Inspector の「ポストプロセス」を外すと、そのカメラには組み込みの効果がかかりません。「Volume の Layer」で、どの Layer の Volume を見るかを選びます。

独自の全画面の効果（白黒など）は、**Project Settings > Graphics > Renderer Features** にマテリアルを並べてかけます。

効果ごとの値、Volume の欄、独自の効果のシェーダの書き方は [シェーダとポストエフェクトの書き方](Shaders.md) にあります。

## ゲームを書き出す

1. Play を停止して **Ctrl+S** でシーンを保存します。Console にスクリプトエラーがあれば直します。
2. **File > Build Settings...** を開きます。**Add Open Scenes** で今のシーンを追加するか、Project の `.scene` を **Scenes In Build** の一覧へドラッグします。
3. チェックを入れたシーンをドラッグで並べます。ビルド番号 **0** のシーンがゲームの起動画面になります。
4. **Player Settings...** で **Product Name** と画面設定を決めます。
5. **Build...** で `Builds/FirstBuild/` のような書き出し専用フォルダーを選びます。再書き出し時は、その中の `Assets/`、`EngineAssets/`、`ProjectSettings/`、`Runtime/` が入れ替わります。
6. 書き出し先から exe を直接起動し、最初のシーン、入力、必要なシーン遷移を確認します。配布するときは、exe と同じ階層の各フォルダーを含め、書き出し先をフォルダーごと渡します。

シーンで `EditorOnly` タグを付けた GameObject とその子は、書き出したシーンから除かれます。タグを付ける前に、ゲーム中に必要なスクリプトや参照がその階層にないか確認してください。

## 困ったとき

| 症状 | 確認すること |
|---|---|
| エディターが起動しない | `Editor/Runtime/` と `Editor/EngineAssets/` が揃っているか確認する。`Editor/Logs/` に GPU やシェーダーのエラーがあれば、対応 GPU とドライバーを確認する。 |
| Scene には見えるが Game に映らない | 有効な Camera があり、位置と向きが対象を捉えているか確認する。 |
| キーを押しても反応しない | Play 中で一時停止が解除されていることを確認し、Game パネルをクリックする。対象の GameObject とスクリプトが有効か、Console にエラーがないかも確認する。 |
| スクリプトを追加できない・動かない | Project の `.as` とクラス名が一致し、`MonoBehavior` を継承しているか確認する。Console のコンパイルエラーは別ファイルに出ているものも直す。 |
| 素材の参照が消えた | アセットと元の `.meta` が両方あるか確認する。別の GUID で作り直された場合は、元の `.meta` を Git から復元するか、Inspector で割り当て直す。 |
| 書き出せない | シーンを保存し、Build Settings に有効なシーンを追加する。`Editor/Player/Player.exe` があるか、書き出し先を専用フォルダーにしたか確認する。 |
| 物がピンクになる・ポストプロセスが効かない | [シェーダとポストエフェクトの書き方の「困ったとき」](Shaders.md#困ったとき)を見る。 |
| 書き出したゲームで問題が出る | `%LOCALAPPDATA%/OusiaEngine/<Product Name>/Logs/` のログを確認する。 |

チームに不具合を伝えるときは、`Editor/version.txt` の内容、操作手順、使ったシーン名、Console またはログの該当箇所を添えます。
