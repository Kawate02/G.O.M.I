CLAUDE.mdに追記・修正する時の基準は `D:/claude/knowledge/notes/CLAUDE.mdに書く基準.md`。

# G.O.M.I

ユーザーの過去作。DxLibを使ったC++製のECS風ゲームエンジン(`GomiEngine/`、Visual Studioソリューション `GomiEngine.sln`)。`portfolio/` はポートフォリオ用ページ。

- DxLib SDKは同梱しない(サードパーティのためコミット対象外)
- ソースはUTF-8とShift-JIS(CP932)が混在。読む時は `iconv -f CP932 -t UTF-8`、書き換える時はエンコードを壊さない

## 必要な時に読むノート(`D:/claude/knowledge/notes/`)

| こんな時 | 読むノート |
|---|---|
| 構造を思い出す・どこを触るか決める | `G.O.M.Iの全体構造(ECS・World・描画の流れ).md` |
| 不具合を直す・挙動がおかしい | `G.O.M.Iの既知の不具合(2026-11-07時点).md` |
| 3D・GUI化の作業を続ける | `G.O.M.Iを3D対応・GUI付きエンジンにする経緯.md` |
