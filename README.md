**決してゴミのようなエンジンという訳ではない。**


world内でcreateEntity()を呼ぶとEntityを取得。

Entityに対してaddComponent()をして機能を追加可能。

ISystemを継承してworldのinitialize()で読み込めばそのworld内でupdateを呼べる。

PlayerControlComponentが用意されており、ActionID(論理キー)とInputKey(実際のキー)のキーマップを
入れれば操作可能。アクションの中身はplayerControlSystemで定義してください。


renderComponentでリソースを指定し、transformComponentを持っていれば画面に表示できる。

## ビルド

必要なもの: Visual Studio 2022(C++ によるデスクトップ開発)、OpenGL 4.5 が動く GPU。
GLFW・miniaudio・stb は CMake が初回の構成時に自動で取ってくる。

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
build/Debug/GomiEngine.exe
```

Visual Studio で開く時は、フォルダを開く(CMake として読み込まれる)か `build/GomiEngine.sln` を開く。

## 動作確認用の起動オプション

人が操作しなくても画面遷移を再現してスクリーンショットを残せる(`GomiEngine/DebugOptions.h`)。

```
GomiEngine.exe --world Field --key 10-60:D --screenshot 60:shot.png --frames 61
```

テストは `ctest --test-dir build -C Debug`。
