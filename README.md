**決してゴミのようなエンジンという訳ではない。**

C++20 の ECS ゲームエンジン。2D と 3D の両方を描ける。描画は OpenGL 4.5(GLFW)、音声は miniaudio、3D モデルは glTF。
ゲームはエンジンと別の「プロジェクトのフォルダ」に置く。中身(シーン・プレハブ・UI・画像・音)は `assets/` のファイル、ゲームごとの Component・System は `game/` の C++ に書き、エンジンを作り直さなくても変えられる。見本は `samples/Demo/`。
シーンはエディタ(`GomiEditor`)で組み立てて保存できる。

## ビルド

必要なもの: Visual Studio 2022(C++ によるデスクトップ開発)、OpenGL 4.5 が動く GPU。
GLFW・miniaudio・stb・glm・cgltf は CMake が初回の構成時に自動で取ってくる。

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
build/Debug/GomiEngine.exe
```

エディタは `build/Debug/GomiEditor.exe`(`--world Field` で最初に開くシーンを指定)。

Visual Studio で開く時は、フォルダを開く(CMake として読み込まれる)か `build/GomiEngine.sln` を開く。
開くプロジェクトは `--project <フォルダ>` で渡す。無ければ exe の隣に `project.json` があるとそこ(書き出したゲーム)、それも無ければ開発中のビルドは見本の `samples/Demo/` を直接開く(JSON を書き換えて起動し直すだけで反映される)。

## プロジェクトのフォルダ

| 場所 | 中身 |
|---|---|
| `project.json` | 最初に開くシーン(`startScene`)、ESC で出すポーズ画面(`pauseUI`) |
| `assets/` | シーン・プレハブ・UI・画像・音(下の「assets の中身」) |
| `game/` | ゲームのコード(`*.cpp` / `*.h`)。CMakeLists は置かない。エディタがエンジンの `sdk/game/CMakeLists.txt` で `build/bin/GomiGame.dll` に作る |
| `tests/replays/` | 再現テスト(操作の記録と、同じ名前の基準のフォルダ) |
| `build/` | game/ のビルドの出力(コミットしない) |

ゲームの exe は exe の隣の `GomiGame.dll`、無ければプロジェクトの `build/bin/GomiGame.dll` を読む。`settings.json`(窓の大きさ・全画面)は遊ぶ人の設定なので exe の隣に置く。

### エンジンをインストールした形

`cmake --install build --config Debug --prefix <先>` で、エディタ・ゲームの exe・DLL と、プロジェクトのゲームのコードを作るための `sdk/`(`GomiEngine.cmake`・`game/CMakeLists.txt`・`include/`・`lib/GomiCore.lib`)が置かれる。バージョンごとの配布物はこの形。ゲームのコードはエンジンと同じ構成(Debug / Release)でしか作れない。cmake は PATH か、Visual Studio に付いてくるもの(vswhere で探す)を使う。

### バージョンごとの配布物

バージョン番号は `CMakeLists.txt` の `project(GomiEngine VERSION <番号>)` の1か所(エディタのタイトル・`--version`・sdk に出る)。プロジェクトの `project.json` の `engine` に使うバージョンを書き、エディタと違えば警告が出る。

`tools/release.sh <番号>` で Release ビルドのインストールした形と見本のプロジェクト(`templates/Demo/`)を `dist/GOMI-<番号>-windows-x64.zip` にし、タグ `v<番号>` を付けて配布用の Private リポジトリ `Kawate02/G.O.M.I-releases` の Releases に上げる(`--no-upload` で zip まで)。番号を上げる時は先に `CMakeLists.txt` を直してコミットする。

### ランチャー(GomiLauncher)

Unity Hub のように、エンジンのバージョンとプロジェクトを1か所で扱う入口。`tools/install-launcher.sh` で Release ビルドを `%USERPROFILE%/GOMI/launcher/` に置く(エンジンのバージョンに依らない1本なので配布物の zip には入れない)。

- 画面: 「バージョン」で Releases から入れる(gh を使う)か、落とした zip から入れる。「プロジェクト」で入れたバージョンのひな形(`templates/`)から作り、`project.json` の `engine` のエディタで開く。入っていない時は入れるか別のバージョンで開くかを選ぶ
- 一覧の行の右クリックか「…」: 設定(名前=フォルダ名・使うバージョン・`startScene`・`pauseUI`。`project.json` のほかの項目は残す)・複製(`build/` を除く。既定の名前は `<名前>_copy`)・エクスプローラー/VS Code で開く・一覧から外す・ごみ箱へ移す。「バージョン」の「消す…」で入れた版を消す(使っているプロジェクトがあれば並べて確かめる。ごみ箱ではなく完全に消す)。エディタなどで開いたままだと名前の変更・ごみ箱・版を消すは断る
- 置き場所: `%USERPROFILE%/GOMI/`(入れたバージョンは `editors/<番号>/`、プロジェクトの一覧は `launcher.json`)。`--home <dir>` で変えられる。`%LOCALAPPDATA%` は Claude デスクトップから動かすと書き込みが別の場所に振り替わるので使わない
- コマンド: 画面と同じことを `--list-releases`・`--install <番号>`・`--install-zip <zip>`・`--new <dir> --engine <番号> [--template <名>]`・`--add <dir>`・`--list-projects`・`--open <dir> [--engine <番号>]`・`--show <dir>`・`--set <dir> <項目>=<値>`・`--set-engine <dir> <番号>`・`--rename <dir> <名前>`・`--duplicate <dir> [--name <名前>]`・`--remove <dir>`・`--trash <dir>`・`--reveal <dir>`・`--code <dir>`・`--uninstall <番号> [--force]`・`--gh-status` でできる(`--help` で一覧)。画面の確認用に `--frames`・`--screenshot <コマ>:<png>`・`--click <コマ>:<x>,<y>`・`--tab projects|versions|new`
- gh が無い・ログインしていない時は、「バージョン」に理由と直し方(`winget install GitHub.cli` / `gh auth login`)が出る。zip からはいつでも入れられる
- エディタ 0.1.0 は日本語などの入ったパスのプロジェクトを開けない(0.1.1 で直した)。新しいプロジェクトの画面で 0.1.0 を選ぶと注意が出る

## assets の中身

アセットは `assets/` からの相対パス(`images/player.png` など)で指定する。

| ファイル | 中身 |
|---|---|
| `scenes/<名前>.json` | 動かす System の並び(この順に毎フレーム呼ぶ)、置く Entity、出す UI |
| `prefabs/*.json` | Entity の雛形(コンポーネントとその値) |
| `ui/*.json` | UI の部品(Text / Image / Button)。ボタンは押した時に出すイベントを書く |
| `images/` `sounds/` | 画像(png 等)と音(wav 等) |
| `models/` | 3D モデル(glTF 2.0 の `.gltf` / `.glb`) |

シーンの例:

```json
{
  "systems": ["PlayerControlSystem", "MoveSystem", "RendererSystem"],
  "entities": [
    { "prefab": "prefabs/player.json", "overrides": { "Transform": { "position": [100, 300, 0] } } },
    { "components": { "Transform": { "position": [400, 300, 0], "scale": [1, 1, 1] },
                      "Renderer": { "path": "images/player.png" } } }
  ],
  "ui": [ { "prefab": "ui/title.json", "pos": [0, 0], "layer": 0 } ]
}
```

- `overrides` はプレハブの最初の Entity に、項目単位で値を重ねる
- 書かなかった項目はコンポーネントの既定値になる
- 知らないコンポーネント名・項目名・enum の名前は、ファイル名と場所を付けたエラーで起動が止まる

## エディタ

`GomiEditor.exe` を起動すると、`project.json` の startScene ではなく新しいシーン(カメラと光だけ)で開く。

| 欄 | できること |
|---|---|
| ヒエラルキー | Entity の一覧(◆ はプレハブから置いたもの)。同じ呼び名は「crate #2」のように番号で区別。上の欄で絞り込み(Ctrl+F)、F2・ダブルクリックで名前を変える。右クリックで名前の変更・JSON で写す・複製・削除・プレハブにする・プレハブを開いて直す。「+ 空の Entity」「+ プレハブ」、アセット欄からプレハブを落としても置ける |
| インスペクタ | 選んだ Entity のコンポーネントと項目。ドラッグ・ダブルクリックで数値入力、enum は選択、画像やモデルの欄は「...」かアセット欄から落とす。× で外す、「コンポーネントを追加」。ゲーム中に変わる値は「実行時の値」に畳まれて読むだけ。プレハブと違う値は名前が青く、右クリックでプレハブの値に戻せる |
| シーン | ゲームの画面。3D はエディタカメラ(右ドラッグで回す・中ドラッグで動かす・ホイール・F で選んだ物へ)。左クリックで物を選び、移動/回転/拡大のギズモ(W/E/R、文字入力中以外ならどこでも効く)で動かす |
| アセット | assets のファイル。シーンはダブルクリックで開く。プレハブはダブルクリックでプレハブを直すモードになる(上の「← シーン に戻る」で戻る。直している間は再生できない) |
| System | シーンで動かす System の並び(上から順)。足す・外す・入れ替える |
| 再現テスト | 操作の記録(`samples/Demo/tests/replays/*.json`)の一覧。流す・全部検査・基準を作り直す。流している間は時間の帯(押すとその場面まで進めて止める。戻る時は 60 フレームごとに覚えた状態から流し直す) |
| 差分 | 保存済みのファイルと今の状態の違いを、Entity・コンポーネント・項目の単位で(緑は追加、赤は削除)。行を押すとその Entity を選ぶ |

- ▶ 再生(F5)でゲームとして動かし、■ 停止で再生前の状態に戻る(再生中の変更は残らない。再生中は帯と枠がオレンジ)。一時停止(F6)・1コマ送り(F7)。「Δt 1/60 固定」なら毎回同じ結果になる
- 下の状態バーに、再生の状態・フレーム・FPS・選択・未保存と、最後の警告/エラー(押すとログを開く)が出る
- Ctrl+Z / Ctrl+Y で元に戻す・やり直す、Ctrl+S で保存、Ctrl+D で複製、Delete で削除
- 2D では選んだスプライトに枠が出て、シーンの画面でドラッグして動かせる(1px 単位。Shift で小数も)
- キー割り当て(PlayerControl.actions)はインスペクタの表で、アクションごとにキーを選ぶ(選ぶ欄で絞り込める)
- Ctrl+C で選んだ Entity をシーンの entities の1項目の JSON で写し、Ctrl+V で JSON(1項目・配列・コンポーネントだけ)を貼って足す。AI に渡す・AI が書いた物を入れる用。間違いがあれば何も足さない
- Ctrl+Shift+P でコマンドパレット。全操作を名前か ID(`scene.open` など)で探して Enter で実行する(↑↓ で選ぶ。最近使った物が上)。シーン・プレハブ・Entity・再現テストを選ぶ物は候補ごとに1行出る。メニューとショートカットも同じコマンドを呼ぶ
- 保存すると、プレハブから置いた物は `prefab` + 変えた項目だけの `overrides` で書かれる。プレハブの2番目以降を変えたり、コンポーネントを外したりすると、つながりを切って中身を全部書く
- 保存していない変更があると、開く・新規・終了の前に確認が出る
- assets の JSON が外で(AI やテキストエディタで)書き換えられたら、0.5 秒ほどで読み直す。プレハブ・UI が変わった時は自分の変更を残したまま作り直す。開いているシーンが変わった時、保存していない変更が無ければそのまま読み直し(Ctrl+Z で前に戻せる)、あれば「外で入った変更」を見せて読み直すか続けるかを聞く。書きかけで読めない時は今のまま。再生中は止めてから読む
- ● 記録: 保存したシーンを最初から再生して操作を記録し、■ 停止で名前を付けて保存する(続けて基準も作れる)。記録はゲームの exe でも同じ結果になり、ctest で流れる

## 3D

座標は右手系・Y が上(glTF と同じ)。回転は度で、`[x: ピッチ, y: ヨー, z: ロール]`。2D のスプライトと UI は 3D の上に画面座標で重なる。

| コンポーネント | 役目 |
|---|---|
| `MeshRenderer` | `mesh` を描く。`builtin:cube` / `builtin:plane` / `builtin:sphere` か `models/*.gltf`。`texture` と `color` を掛ける |
| `Camera3D` | このカメラから見る(回転 0 で -Z を向く)。`fov`・`clearColor`(背景色) |
| `Light` | 平行光源。`direction`(光が進む向き)・`color`・`ambient` |
| `Collision` | `BOX`(position + offset を角に size の箱)か `SPHERE`(中心 position + offset)。回転・拡大には追従しない |
| `PlayerControl` | `plane: "XZ"` で上下キーが奥・手前になる。`jumpSpeed` |
| `Physics` | `gravityDirection: [0, -1, 0]` で 3D の下向き |

3D のシーンでは `MeshRendererSystem` を、カメラを追従させるなら `CameraSystem` も並べる。例は `samples/Demo/assets/scenes/Demo3D.json`(タイトルの「3D DEMO」から開く)。

## コンポーネント・System を足す

- **コンポーネント**: データだけの構造体を作り、`GomiEngine/ComponentRegistration.cpp` に名前と項目を登録する
  (`r.add<MyComponent>("My").field("speed", &MyComponent::speed)`)。数値・bool・文字列・Vec2・Vec3・名前表を持つ enum・それらの map が使える
- **System**: `ISystem` を継承し、`GomiEngine/SystemRegistry.cpp` に名前を登録する。シーンの `systems` にその名前を書けば動く
- enum を JSON で使う時は `GOMI_ENUM_NAMES` で名前表を付ける(`EnumNames.h`)

## 動作確認用の起動オプション

人が操作しなくても画面遷移を再現してスクリーンショットを残せる(一覧は `GomiEngine/DebugOptions.h`)。

```
GomiEngine.exe --world Field --fixed-dt 60 --key 10-60:D --screenshot 60:shot.png --frames 61
GomiEngine.exe --world Field --save-scene 1:saved.json --frames 1   # 今の World をシーンとして保存
GomiEngine.exe --world saved.json                                   # 保存したシーンを開く
GomiEditor.exe --version                                            # エンジンのバージョンを出して終わる
GomiEditor.exe --list-commands                                      # エディタのコマンドの一覧(ID・ショートカット・名前・引数をタブ区切り)
GomiEditor.exe --world Field --command 30:entity.select=player --command 31:entity.duplicate --screenshot 40:shot.png --frames 41
GomiEditor.exe --world Field --type "30:{CTRL+SHIFT+P}" --type "40:複製" --type "50:{ENTER}"   # キーも押せる
```

テストは `ctest --test-dir build -C Debug`。

### 再現テスト(操作の記録と再生)

操作を記録した JSON を流し、確認点のフレームで画面(png)と状態(World を保存した JSON)を基準と比べる。同じ入力・固定のΔt・同じ乱数の種なので毎回同じになり、違えば終了コード 1 と、違う項目・差分の画像(違う画素が赤)を出す。

```
GomiEngine.exe --project P --record P/tests/replays/walk.json                  # 遊んで操作を記録(Δt 1/60 固定)。閉じると保存
GomiEngine.exe --project P --replay P/tests/replays/walk.json --replay-update   # 今の動きを基準にする(P/tests/replays/walk/ に保存)
GomiEngine.exe --project P --replay P/tests/replays/walk.json --no-wait        # 流して比べる(ctest は見本の記録を Replay.walk として流す)
```

- 記録の JSON は1フレーム1行で、手で直してよい(`checkpoints` に確認点のフレームを足す、など)
- 結果は一時フォルダの `gomi-replay/<名前>/` に出る(`--replay-out` で変えられる)
- 画面を変える変更をしたら、確かめてから `--replay-update`(エディタでは「基準を作り直す」)で基準を更新する
