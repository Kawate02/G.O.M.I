CLAUDE.mdに追記・修正する時の基準は `D:/claude/knowledge/notes/CLAUDE.mdに書く基準.md`。

# G.O.M.I

ユーザーの過去作の、C++製のECSゲームエンジン(`GomiEngine/`)とImGuiのエディタ(`Editor/`)。OpenGL 4.5(GLFW)・miniaudio・CMake。`portfolio/` はポートフォリオ用のページ。使い方・仕様は下の表の仕様ノート(`kb.py -p G.O.M.I -k 参照 仕様` で一覧)。

## 守ること
- コードを変えたら `tools/verify.sh` を流す(変更前にも一度流して基準を撮る)。`ctest` も通す。見た目や動きを意図して変えた時は、画面を確かめてから `--replay-update` で基準を更新する
- エディタの中身は `SceneEditor`(画面無し・テストあり)に書き、画面は `Editor/EditorApp.cpp`。操作を足す時は `registerCommands()` にコマンドとして足す
- ゲームの中身(シーン・プレハブ・UI・画像・音)は `assets/` のJSONとファイルに置き、C++に静的に書かない。ゲームごとのComponent・Systemは `game/` に書く。エンジン組み込みのものだけ `ComponentRegistration.cpp` / `SystemRegistry.cpp` に足す
- ソースは UTF-8(MSVC に `/utf-8` を付けている)。Shift-JIS で保存しない
- `build/` はコミットしない。外部ライブラリは CMake の FetchContent で取り、リポジトリに入れるのは生成済みの glad だけ
- エディタの動作確認で保存を試す時は見本のプロジェクトのコピーを `--project` で渡す(コピーは `%TEMP%` や `AppData\Local` の下に置かない)

## 必要な時に読むノート(`D:/claude/knowledge/notes/`)

| こんな時 | 読むノート |
|---|---|
| ビルドする・動作確認する・変更後に確かめる | `G.O.M.Iを変えたらtools-verify-shでビルドと画面と再現テストを確かめる.md` |
| ゲームのプロジェクト(assets・project.json・見本)を作る・直す | `G.O.M.Iでゲームを作るにはプロジェクトのフォルダにassetsのJSONとgameのコードを置く.md` |
| ゲームのコード(game/・GomiGame.dll)を書く・試す | `G.O.M.Iのゲームのコードはプロジェクトのgame-に書きエディタを開いたまま試す.md` |
| エディタを使う・操作を足す | `G.O.M.Iのエディタを使うには--projectでプロジェクトを渡し操作は全部コマンドのIDで呼ぶ.md` |
| 操作を記録して再現テストにする | `G.O.M.Iの操作の記録はGomiEngineの--recordと--replayで流して基準と比べる.md` |
| ランチャー(版・プロジェクト管理) | `G.O.M.Iのエンジンの版とプロジェクトを扱うにはGomiLauncherを使う.md` |
| 配布物を作って上げる | `G.O.M.Iの配布物はtools-release-shで作りPrivateのReleasesに上げる.md` |
| エディタの UI・機能を足す・直す(コンセプト・優先度・配置) | `G.O.M.I/docs/editor-design.md`(リポジトリ内) |
| 構造を思い出す・どこを触るか決める | `G.O.M.Iの全体構造(ECS・World・描画の流れ).md` |
| シーン・プレハブ・アセットの形式を変える | `G.O.M.Iのデータ形式はパスID・厳格チェック・項目単位の上書きにした.md` |
| エディタを触る | `G.O.M.Iのエディタは画面無しの中身とImGuiの画面に分けた.md` |
| 3D(座標・回転・当たり判定・描画)を触る | `G.O.M.Iの3D化はY上の右手系・度の回転・2DをZ0の3Dとして扱う形にした.md` |
| 不具合を直す・挙動がおかしい | `G.O.M.Iの既知の不具合(2026-11-07時点).md` |
