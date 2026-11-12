#!/usr/bin/env bash
# ランチャー(GomiLauncher)を Release でビルドし、置き場所の launcher/ に置く(Git Bash 用)。
# ランチャーはエンジンのバージョンに依らない1本なので、配布物の zip には入れずここで置く。
#
# 使い方: tools/install-launcher.sh [<置き場所>]
#   <置き場所>  既定は %USERPROFILE%/GOMI(ランチャーの --home の既定と同じ)
# 置くもの: <置き場所>/launcher/GomiLauncher.exe・glfw3.dll
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CMAKE="/c/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
BUILD="$ROOT/build"
HOME_DIR="${1:-$(cygpath -u "$USERPROFILE")/GOMI}"
DEST="$HOME_DIR/launcher"

if [ ! -f "$BUILD/CMakeCache.txt" ]; then
  "$CMAKE" -S "$ROOT" -B "$BUILD" -G "Visual Studio 17 2022" -A x64 > /dev/null
fi
echo "Release でビルドしています…"
if ! "$CMAKE" --build "$BUILD" --config Release --target GomiLauncher --parallel > "$BUILD/launcher-release.txt" 2>&1; then
  echo "ビルド: 失敗 ($BUILD/launcher-release.txt)"
  grep -E "error|エラー" "$BUILD/launcher-release.txt" | grep -v "_deps" | sort -u | head -8
  exit 1
fi

mkdir -p "$DEST"
cp "$BUILD/Release/GomiLauncher.exe" "$BUILD/Release/glfw3.dll" "$DEST/"
echo "置きました: $DEST/GomiLauncher.exe"
