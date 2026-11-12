#!/usr/bin/env bash
# 変更後の動作確認を1コマンドで行う(Git Bash 用)。
#   Debug ビルド → --fixed-dt 60 で操作を再現して撮影 → 基準の画面と cmp で比べる → ctest
# 出力は要約だけ。詳しいログは verify/logs/ に残す。
#
# 使い方: tools/verify.sh [オプション]
#   --update      今回撮った画面で基準を置き換える(見た目を意図して変えた時。差分を見てから)
#   --no-build    ビルドを飛ばす
#   --no-ctest    ctest を飛ばす
#   --only <名前> その場面だけ流す(game / editor)
# 基準が無い場面は、今回撮った物を基準として保存する(変更前に一度流しておく)。
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CMAKE="/c/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
CTEST="$(dirname "$CMAKE")/ctest.exe"
BUILD="$ROOT/build"
BIN="$BUILD/Debug"
OUT="$ROOT/verify"
LOGS="$OUT/logs"

UPDATE=0; DO_BUILD=1; DO_CTEST=1; ONLY=""
while [ $# -gt 0 ]; do
  case "$1" in
    --update) UPDATE=1 ;;
    --no-build) DO_BUILD=0 ;;
    --no-ctest) DO_CTEST=0 ;;
    --only) ONLY="$2"; shift ;;
    -h|--help) sed -n '2,11p' "$0"; exit 0 ;;
    *) echo "不明なオプション: $1"; exit 2 ;;
  esac
  shift
done

mkdir -p "$LOGS"
FAIL=0

# ---- ビルド ----
if [ $DO_BUILD -eq 1 ]; then
  if [ ! -f "$BUILD/CMakeCache.txt" ]; then
    if ! "$CMAKE" -S "$ROOT" -B "$BUILD" -G "Visual Studio 17 2022" -A x64 > "$LOGS/configure.txt" 2>&1; then
      echo "構成: 失敗 ($LOGS/configure.txt)"; grep -i "error" "$LOGS/configure.txt" | head -8; exit 1
    fi
  fi
  if "$CMAKE" --build "$BUILD" --config Debug --parallel > "$LOGS/build.txt" 2>&1; then
    WARN=$(grep -i " warning " "$LOGS/build.txt" | grep -v "_deps\|third_party" | sort -u | wc -l)
    echo "ビルド: 成功 (自前コードの警告 ${WARN}件)"
  else
    echo "ビルド: 失敗 ($LOGS/build.txt)"
    grep -E "error|エラー" "$LOGS/build.txt" | grep -v "_deps\|third_party" | sort -u | head -8
    exit 1
  fi
fi

# ---- 場面の定義 ----
# 各場面は <exe> と起動オプション。撮影先は {D}(場面の出力フォルダ)、見本のプロジェクトのコピーは {P}
scene_args() {
  case "$1" in
    game) # タイトル→フィールド→移動→ポーズ→再開→タイトルの一巡
      echo GomiEngine.exe --project "{P}" --fixed-dt 60 \
        --screenshot "30:{D}/1.png" --click "40:400,400" --screenshot "70:{D}/2.png" \
        --key "80-140:D" --screenshot "140:{D}/3.png" --key "150:ESCAPE" --screenshot "160:{D}/4.png" \
        --click "170:400,300" --screenshot "180:{D}/5.png" --key "190:ESCAPE" --click "200:400,400" \
        --screenshot "230:{D}/6.png" --frames 240 ;;
    editor) # Demo3D を開いて数値欄をドラッグ・入力
      echo GomiEditor.exe --project "{P}" --world Demo3D --fixed-dt 60 --select 0 \
        --click 20:750,440 --drag 40-60:782,430,900,430 --type "26:{SELECTALL}" --type 27:300 \
        --type "28:{ENTER}" --screenshot "70:{D}/1.png" --frames 71 ;;
  esac
}
SCENES="game editor"
[ -n "$ONLY" ] && SCENES="$ONLY"

# ---- 撮影と比較 ----
for s in $SCENES; do
  cur="$OUT/current/$s"; base="$OUT/baseline/$s"
  rm -rf "$cur"; mkdir -p "$cur"
  project="$OUT/project-copy"
  rm -rf "$project"; mkdir -p "$project"
  cp -r "$ROOT/samples/Demo/project.json" "$ROOT/samples/Demo/assets" "$ROOT/samples/Demo/game" "$project/"
  D=$(cygpath -m "$cur"); P=$(cygpath -m "$project")
  args=$(scene_args "$s")
  [ -z "$args" ] && { echo "$s: 不明な場面"; FAIL=1; continue; }
  args=${args//\{D\}/$D}; args=${args//\{P\}/$P}
  # shellcheck disable=SC2086
  eval "set -- $args"
  exe="$1"; shift
  ( cd "$BIN" && "./$exe" "$@" ) > "$LOGS/$s.txt" 2>&1
  code=$?
  shots=$(ls "$cur"/*.png 2>/dev/null | wc -l)
  if [ $code -ne 0 ] || [ "$shots" -eq 0 ]; then
    echo "$s: 起動失敗 (終了コード $code、撮影 ${shots}枚、$LOGS/$s.txt)"
    tail -5 "$LOGS/$s.txt"; FAIL=1; continue
  fi
  if [ $UPDATE -eq 1 ] || [ ! -d "$base" ]; then
    why="基準を保存"; [ -d "$base" ] && why="基準を更新"
    rm -rf "$base"; mkdir -p "$base"; cp "$cur"/*.png "$base/"
    echo "$s: ${why} (${shots}枚)"; continue
  fi
  same=0; diffs=""
  for f in "$cur"/*.png; do
    n=$(basename "$f")
    if [ -f "$base/$n" ] && cmp -s "$f" "$base/$n"; then same=$((same+1)); else diffs="$diffs $n"; fi
  done
  for f in "$base"/*.png; do [ -f "$cur/$(basename "$f")" ] || diffs="$diffs $(basename "$f")(今回なし)"; done
  if [ -z "$diffs" ]; then
    echo "$s: 一致 ${same}/${shots}"
  else
    echo "$s: 不一致${diffs} (一致 ${same}/${shots})"
    echo "  今回: $cur/  基準: $base/"; FAIL=1
  fi
done
rm -rf "$OUT/project-copy"

# ---- ctest ----
if [ $DO_CTEST -eq 1 ]; then
  ( cd "$BUILD" && "$CTEST" -C Debug --output-on-failure ) > "$LOGS/ctest.txt" 2>&1
  summary=$(grep -E "tests passed|tests failed" "$LOGS/ctest.txt" | tail -1)
  if grep -q "100% tests passed" "$LOGS/ctest.txt"; then
    echo "ctest: $summary"
  else
    echo "ctest: ${summary:-実行失敗} ($LOGS/ctest.txt)"
    grep -E "^\s+[0-9]+ - " "$LOGS/ctest.txt" | head -10; FAIL=1
  fi
fi

exit $FAIL
