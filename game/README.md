# game/ — ゲームのコード

ここの `*.cpp` / `*.h` が `GomiGame.dll` になる。エンジン(`GomiEngine/`)を触らずに、ゲームごとの Component と System を足す場所。

- `#include "GameCode.h"` し、同じファイルに `GOMI_REGISTER { ... }` で登録する(書き方は `GomiEngine/GameCode.h` の先頭)
- エディタを開いたまま保存すると、エディタがビルドして読み直す(止めている時に差し替わる。再生中なら停止した時)
- ゲームの exe(GomiEngine.exe)は起動時に隣の `GomiGame.dll` を読む
- 組み込みと同じ名前は登録できない。シーンで使っているコンポーネント・項目の名前を変えると、そのシーンが読めなくなる(エディタは前のコードに戻して理由を出す)
