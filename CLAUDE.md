CLAUDE.mdに追記・修正する時の基準は `D:/claude/knowledge/notes/CLAUDE.mdに書く基準.md`。

# G.O.M.I

ユーザーの過去作の、C++製のECSゲームエンジン(`GomiEngine/`)。3D対応・GUIエディタ付きのエンジンにするのが目標。描画は OpenGL 4.5(GLFW)、音声は miniaudio、ビルドは CMake(2026-11-07 に DxLib から移行)。`portfolio/` はポートフォリオ用のページ。

- コードを変えたら Debug ビルドし、起動オプション(`--screenshot` など)で操作を再現して撮った画面を見て確認する。`ctest` も通す
- ソースは UTF-8(MSVC に `/utf-8` を付けている)。Shift-JIS で保存しない
- `build/` はコミットしない。外部ライブラリは CMake の FetchContent で取り、リポジトリに入れるのは生成済みの glad だけ

## 必要な時に読むノート(`D:/claude/knowledge/notes/`)

| こんな時 | 読むノート |
|---|---|
| ビルドする・動作確認する | `G.O.M.Iのビルドと動作確認の手順.md` |
| 構造を思い出す・どこを触るか決める | `G.O.M.Iの全体構造(ECS・World・描画の流れ).md` |
| 不具合を直す・挙動がおかしい | `G.O.M.Iの既知の不具合(2026-11-07時点).md` |
| 3D・GUI化の作業を続ける | `G.O.M.Iを3D対応・GUI付きエンジンにする経緯.md` |
