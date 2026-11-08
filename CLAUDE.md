CLAUDE.mdに追記・修正する時の基準は `D:/claude/knowledge/notes/CLAUDE.mdに書く基準.md`。

# G.O.M.I

ユーザーの過去作の、C++製のECSゲームエンジン(`GomiEngine/`)。2Dと3Dを描け、ImGuiのエディタ(`Editor/`、`GomiEditor`)でシーンを組み立てられる(2026-11-08 に3D・エディタ対応が完了)。描画は OpenGL 4.5(GLFW)、音声は miniaudio、ビルドは CMake(2026-11-07 に DxLib から移行)。`portfolio/` はポートフォリオ用のページ。

- コードを変えたら Debug ビルドし、`--fixed-dt 60` 付きの起動オプションで操作を再現して撮った画面を、変更前の画面と比べて確認する。`ctest` も通す。エディタも同じ起動オプション(`--click`・`--drag`・`--type`)で操作を再現して撮る。エディタで保存を試す時は assets のコピーを `--assets` で渡す
- エディタの中身(編集・元に戻す・再生/停止・保存)は `SceneEditor`(画面無し・テストあり)、画面は `Editor/EditorApp.cpp`。編集の処理は SceneEditor 側に書く
- ゲームの中身(シーン・プレハブ・UI・画像・音)は `assets/` の JSON とファイル。C++ に静的に書かない。新しいコンポーネント・System は登録ファイル(`ComponentRegistration.cpp` / `SystemRegistry.cpp`)に足す
- ソースは UTF-8(MSVC に `/utf-8` を付けている)。Shift-JIS で保存しない
- `build/` はコミットしない。外部ライブラリは CMake の FetchContent で取り、リポジトリに入れるのは生成済みの glad だけ

## 必要な時に読むノート(`D:/claude/knowledge/notes/`)

| こんな時 | 読むノート |
|---|---|
| エディタの UI・機能を足す・直す(コンセプト・優先度・配置) | `G.O.M.I/docs/editor-design.md`(リポジトリ内) |
| ビルドする・動作確認する | `G.O.M.Iのビルドと動作確認の手順.md` |
| 構造を思い出す・どこを触るか決める | `G.O.M.Iの全体構造(ECS・World・描画の流れ).md` |
| シーン・プレハブ・アセットの形式を変える | `G.O.M.Iのデータ形式はパスID・厳格チェック・項目単位の上書きにした.md` |
| エディタを触る | `G.O.M.Iのエディタは画面無しの中身とImGuiの画面に分けた.md` |
| 3D(座標・回転・当たり判定・描画)を触る | `G.O.M.Iの3D化はY上の右手系・度の回転・2DをZ0の3Dとして扱う形にした.md` |
| 不具合を直す・挙動がおかしい | `G.O.M.Iの既知の不具合(2026-11-07時点).md` |
