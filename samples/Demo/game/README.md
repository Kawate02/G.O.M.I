# game/ — ゲームのコード

ここの `*.cpp` / `*.h` が `GomiGame.dll` になる。エンジンを触らずに、ゲームごとの Component と System を足す場所。CMakeLists は要らない(エディタがエンジンの `sdk/game/CMakeLists.txt` でプロジェクトの `build/` に作る)

- `#include "GameCode.h"` し、同じファイルに `GOMI_REGISTER { ... }` で登録する(書き方は `GomiEngine/GameCode.h` の先頭)
- エディタを開いたまま保存すると、エディタがビルドして読み直す(止めている時に差し替わる。再生中なら停止した時)
- エディタの「コード」メニュー: ゲームのコードを開く(VS Code)・新しい Component / System / Resource のひな形を作る・ビルドし直す。`--command code.newComponent=Hp` のようにも呼べる
- ビルドの状態は状態バーの「コード: …」に出る(ビルド中・エラー n 件・停止すると読み直す・最新)。エラーの全文はログ
- 足した Component・System は、インスペクタの「コンポーネントを追加」と System タブの「System を追加」(コマンドなら `component.add` / `system.add`)の候補に出る
- シーンをまたいで残るデータ(フラグ・ステータス・持ち物)は Resource にする: 構造体を `resources.add<型>("名前").field(...)` で登録し、System から `world.resource<型>()` で読み書きする。初期値はエディタの Resource パネルで直し `assets/resources.json` に保存する。入れ子の構造体は直後に `GOMI_FIELDS(型, 項目, ...)` を書く。HP や持ち物の決まりはエンジンには無いので、ここに書く
- 触れた時の処理: タグを `project.json` の `tags`、ルールを `assets/collision_rules.json`(`"kind": "TRIGGER"`)に書くと、`CollisionSystem` が重なった両方に `CollisionEvent`(rule・ENTER/STAY/EXIT)を出す。System で `world.event().collisions()` を回して書く(シーンの systems で CollisionSystem より後に置く。見本は `PickupSystem.cpp`)
- ゲームの exe(GomiEngine.exe)は起動時に、exe の隣の `GomiGame.dll`、無ければプロジェクトの `build/bin/GomiGame.dll` を読む
- 組み込みと同じ名前は登録できない。シーンで使っているコンポーネント・項目の名前を変えると、そのシーンが読めなくなる(エディタは前のコードに戻して理由を出す)
