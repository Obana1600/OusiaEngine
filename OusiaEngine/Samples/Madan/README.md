# 魔弾（仮） スクリプト

7 発の魔弾で体力 7 のボスを倒す、避けメインのボス戦のスクリプトです。
当たり判定は物理エンジンを使わず、XZ 平面上の距離でスクリプトが計算します（高さは無視）。

## 開き方

`Game` のエディタを `-projectPath $(SolutionDir)Samples\Madan` 付きで起動します（詳しくは [Samples/README.md](../README.md)）。
シーンはまだないので、下の「シーンの組み方」で `Assets/Scenes/` に作り、Build Settings に入れてください。

```
Samples/Madan/
  Assets/
    Scenes/        … シーン（まだ空）
    Script/        … このゲームの AngelScript
  ProjectSettings/ … Game/ProjectSettings から TagManager・DynamicsManager を写したもの
```

## スクリプト（`Assets/Script/`）

| ファイル | 付ける場所 | 役割 |
| --- | --- | --- |
| `MadanCommon.as` | （付けない） | 弾の種類の enum、共通の関数 |
| `MadanBattle.as` | 空の GameObject（`Battle`） | 勝敗の判定、HUD、リスタート |
| `MadanPlayer.as` | プレイヤー（名前 `Player`） | 移動、回避、照準、被弾 |
| `MagicGun.as` | プレイヤー（同じ GameObject） | 弾の並び、残弾、発射、弾ごとの数値 |
| `MagicBullet.as` | （自動で付く） | 撃った魔弾 1 発の動きと当たり判定 |
| `MadanBoss.as` | ボス（名前 `Boss`） | 体力、段階の進行、攻撃 |
| `WeakPoint.as` | ボスの子の GameObject | 弱点。段階に合わせて開閉する |
| `BossProjectile.as` | （自動で付く） | ボスの弾 |
| `QuarterViewCamera.as` | カメラ | 見下ろしのカメラ |

## シーンの組み方

1. 床（Plane など）を置く。アリーナは原点中心、半径 `MadanBattle.arenaRadius`（12）の円です。
2. Capsule を作って名前を `Player` にし、`MadanPlayer` と `MagicGun` を付ける。
   - `MagicGun.bulletPrefab` に小さな Sphere の Prefab を割り当てる（Collider は外しておく）。
   - カーソル位置に照準を出したいときは、薄い Cylinder などを `MadanPlayer.aimMarker` に割り当てる（Collider は外す）。
3. ボスの体（大きめの Sphere など）を作って名前を `Boss` にし、`MadanBoss` を付ける。
   - `projectilePrefab` に小さな Sphere の Prefab を割り当てる。
   - `bodyRadius` を見た目の半径に合わせる（選択するとギズモで円が出ます）。
4. ボスの子に小さな Sphere を 7 個置き、それぞれ `WeakPoint` を付けて `stages` に `0`〜`6` を入れる。
   - 弱点は体の表面より少し外に出す（体の当たり判定の中に埋まると、弾が先に体に当たります）。
   - 同じ番号を複数の弱点に入れると同時に露出します。`2, 5` のように書くと 2 回使えます。
5. 空の GameObject `Battle` に `MadanBattle` を付ける。HUD 用の Text があれば `hudText` / `messageText` に割り当てる。
6. カメラに `QuarterViewCamera` を付ける。カーソルで狙うときは、このカメラが使われる（タグが MainCamera のカメラ。なければ最初に動いているカメラ）。

## 操作

| 操作 | キー |
| --- | --- |
| 移動 | WASD（カメラの向きが基準） |
| 狙う | マウスカーソル（常にカーソルの方を向く） |
| 撃つ | 左クリック |
| 回避 | Space / 左 Shift |
| 弾を選ぶ | Q / E（`MagicGun.allowSelect` が true のとき） |
| リトライ | 決着後に R / Enter |

## 魔弾の性質（仮）

`MagicGun.bulletOrder` に名前を並べて順番を決めます。

| 名前 | 性質 |
| --- | --- |
| `Normal` | まっすぐ飛ぶ |
| `Pierce` | ボスの体を貫通し、裏側の弱点にも届く |
| `Homing` | 露出中の弱点へ曲がる |
| `Large` | 当たり判定が大きい |
| `Recall` | 外れると一度だけ手元に戻る（戻った弾は通常弾になる） |
| `Split` | 3 方向に分かれる。どれか 1 発が当たれば 1 ダメージ |
| `Devil` | 露出中の弱点に必ず当たる。露出中の弱点がなければ撃った本人に当たる |
