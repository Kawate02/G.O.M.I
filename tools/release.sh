#!/usr/bin/env bash
# エンジンのバージョンごとの配布物(zip)を作り、配布用の Private リポジトリの GitHub Releases に上げる(Git Bash 用)。
#   Release でビルド → cmake --install → 見本のプロジェクトを templates/ に入れる → zip → タグ v<番号> → gh release create
#
# 使い方: tools/release.sh <番号> [--no-upload]
#   <番号>        CMakeLists.txt の project(GomiEngine VERSION <番号>) と同じもの(番号を上げるのは先にコミットする)
#   --no-upload   zip を作るところまで(タグも Releases も作らない)
# 出来るもの: dist/GOMI-<番号>-windows-x64.zip(中身は dist/GOMI-<番号>/)
#   直下: GomiEditor.exe・GomiEngine.exe・*.dll・settings.json
#   sdk/: ゲームのコードをビルドする CMake の設定・ヘッダ・GomiCore.lib
#   templates/Demo/: 見本のプロジェクト(新しいプロジェクトのひな形)
# 上げ先: Kawate02/G.O.M.I-releases(本体のリポジトリは Public なので、配布物は別の Private リポジトリに置く)
set -eu

RELEASE_REPO="Kawate02/G.O.M.I-releases"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CMAKE="/c/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
BUILD="$ROOT/build"
DIST="$ROOT/dist"

VERSION=""; UPLOAD=1
while [ $# -gt 0 ]; do
  case "$1" in
    --no-upload) UPLOAD=0 ;;
    -h|--help) sed -n '2,13p' "$0"; exit 0 ;;
    -*) echo "不明なオプション: $1"; exit 2 ;;
    *) VERSION="$1" ;;
  esac
  shift
done
[ -n "$VERSION" ] || { echo "使い方: tools/release.sh <番号> [--no-upload]"; exit 2; }

# ---- 番号と作業ツリーを確かめる ----
CMAKE_VERSION=$(sed -n 's/^project(GomiEngine VERSION \([0-9.]*\) .*/\1/p' "$ROOT/CMakeLists.txt" | tr -d '\r')
if [ "$VERSION" != "$CMAKE_VERSION" ]; then
  echo "番号が CMakeLists.txt の project VERSION($CMAKE_VERSION)と違います。先に CMakeLists.txt を直してコミットしてください"; exit 1
fi
TAG="v$VERSION"
if [ $UPLOAD -eq 1 ]; then
  if [ -n "$(git -C "$ROOT" status --porcelain)" ]; then
    echo "コミットしていない変更があります。配布物はコミットした状態から作ります"; exit 1
  fi
  if gh release view "$TAG" -R "$RELEASE_REPO" > /dev/null 2>&1; then
    echo "$RELEASE_REPO に $TAG はもうあります。上書きはしません(番号を上げてください)"; exit 1
  fi
fi

# ---- Release でビルドしてインストールした形を作る ----
mkdir -p "$DIST"
if [ ! -f "$BUILD/CMakeCache.txt" ]; then
  "$CMAKE" -S "$ROOT" -B "$BUILD" -G "Visual Studio 17 2022" -A x64 > "$DIST/configure.txt" 2>&1 \
    || { echo "構成: 失敗 ($DIST/configure.txt)"; exit 1; }
fi
# 番号を変えた直後でも GomiVersion.h を作り直させる
"$CMAKE" "$BUILD" > "$DIST/configure.txt" 2>&1 || { echo "構成: 失敗 ($DIST/configure.txt)"; exit 1; }
"$CMAKE" --build "$BUILD" --config Release --parallel --target GomiEngine GomiEditor > "$DIST/build.txt" 2>&1 \
  || { echo "ビルド: 失敗 ($DIST/build.txt)"; grep -E "error|エラー" "$DIST/build.txt" | head -8; exit 1; }
echo "ビルド: 成功 (Release)"

NAME="GOMI-$VERSION"
OUT="$DIST/$NAME"
ZIP="$DIST/$NAME-windows-x64.zip"
rm -rf "$OUT" "$ZIP"
"$CMAKE" --install "$BUILD" --config Release --prefix "$OUT" > "$DIST/install.txt" 2>&1 \
  || { echo "インストール: 失敗 ($DIST/install.txt)"; exit 1; }

# 見本のプロジェクト(build/ は入れない)
mkdir -p "$OUT/templates"
cp -r "$ROOT/samples/Demo" "$OUT/templates/Demo"
rm -rf "$OUT/templates/Demo/build"

GOT=$("$OUT/GomiEditor.exe" --version | tr -d '\r')
[ "$GOT" = "$VERSION" ] || { echo "出来たエディタの番号が $GOT でした($VERSION のはず)"; exit 1; }

# zip の中は GOMI-<番号>/ 1つにする(展開した場所にそのまま1フォルダで並ぶ)
python -I -c "import shutil,sys; shutil.make_archive(sys.argv[1], 'zip', sys.argv[2], sys.argv[3])" \
  "${ZIP%.zip}" "$DIST" "$NAME"
echo "zip: $ZIP ($(du -h "$ZIP" | cut -f1))"

[ $UPLOAD -eq 1 ] || exit 0

# ---- タグと Releases ----
COMMIT=$(git -C "$ROOT" rev-parse HEAD)
if ! git -C "$ROOT" rev-parse -q --verify "refs/tags/$TAG" > /dev/null; then
  git -C "$ROOT" tag "$TAG"
fi
git -C "$ROOT" push origin "refs/tags/$TAG"
gh release create "$TAG" "$ZIP" -R "$RELEASE_REPO" --title "G.O.M.I $VERSION" \
  --notes "G.O.M.I エンジン $VERSION(Kawate02/G.O.M.I の $COMMIT、Release ビルド)"
echo "Releases: $RELEASE_REPO $TAG"
