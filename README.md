**決してゴミのようなエンジンという訳ではない。**

C++20 の ECS ゲームエンジン。2D と 3D の両方を描ける。描画は OpenGL 4.5(GLFW)、音声は miniaudio、3D モデルは glTF。
ゲームの中身(シーン・プレハブ・UI・画像・音)は `assets/` のファイルに書き、エンジンを作り直さなくても変えられる。

## ビルド

必要なもの: Visual Studio 2022(C++ によるデスクトップ開発)、OpenGL 4.5 が動く GPU。
GLFW・miniaudio・stb・glm・cgltf は CMake が初回の構成時に自動で取ってくる。

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
build/Debug/GomiEngine.exe
```

Visual Studio で開く時は、フォルダを開く(CMake として読み込まれる)か `build/GomiEngine.sln` を開く。
開発中は exe の隣に `assets` が無ければリポジトリの `assets/` を直接読むので、JSON を書き換えて起動し直すだけで反映される。

## assets の中身

アセットは `assets/` からの相対パス(`images/player.png` など)で指定する。

| ファイル | 中身 |
|---|---|
| `project.json` | 最初に開くシーン(`startScene`)、ESC で出すポーズ画面(`pauseUI`) |
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

3D のシーンでは `MeshRendererSystem` を、カメラを追従させるなら `CameraSystem` も並べる。例は `assets/scenes/Demo3D.json`(タイトルの「3D DEMO」から開く)。

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
```

テストは `ctest --test-dir build -C Debug`。
