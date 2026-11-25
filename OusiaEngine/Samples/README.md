# サンプルのプロジェクト

「このエンジンでこんなゲームが作れる」を見せるためのプロジェクトを置く。1 つのフォルダが 1 つのプロジェクトで、中身はチームのリポジトリと同じく `Assets/` と `ProjectSettings/` だけ（C++ のプロジェクトは作らない。ゲームのコードは AngelScript）。

```
Samples/
  <ゲームの名前>/
    Assets/
    ProjectSettings/
```

## 開き方

`Game` のプロジェクトのエディタ（Debug / Develop）を `-projectPath` 付きで起動する。VS では `Game` のプロパティの「デバッグ」→「コマンド引数」に次のように書く（`.vcxproj.user` に入るので、人ごとの設定になる）。

```
-projectPath $(SolutionDir)Samples\<ゲームの名前>
```

## 新しいサンプルを作る

1. `Samples/<ゲームの名前>/Assets/Scenes/` を作る（空のフォルダは git に残らないので、シーンを置くまでは `.gitkeep` を置く）
2. `ProjectSettings/` には `Game/ProjectSettings/` の `TagManager.json`・`DynamicsManager.json` を写す（ほかの設定はエディタが作る）
3. 上の `-projectPath` で開き、シーンを作って Build Settings に入れる

`UserSettings/`・`as.predefined` は起動するたびにエディタが作るので、コミットしない（`.gitignore` で外している）。
