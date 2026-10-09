#!/usr/bin/env bash
# 在電腦上跑遊戲並產生 README 用的 GIF:tools/capture/run.sh [key...](沒給 key 就全部)
# 需要 g++、ffmpeg、SDL2,以及先跑過一次 pio run 抓好的 M5GFX / M5Unified
set -euo pipefail
cd "$(dirname "$0")/../.."
LIB=.pio/libdeps/m5stack-core2
OBJ=.pio/capture/obj
mkdir -p "$OBJ"
INC=(-Itools/capture/shim -I"$LIB/M5GFX/src" -I"$LIB/M5Unified/src" $(sdl2-config --cflags))
FLAGS=(-O2 -std=c++17 -w "${INC[@]}")

# 函式庫只編一次
find "$LIB/M5GFX/src" "$LIB/M5Unified/src" \( -name '*.cpp' -o -name '*.c' \) | while read -r f; do
  o="$OBJ/$(echo "${f#$LIB/}" | tr / _).o"; [ -f "$o" ] || echo "$f $o"
done | xargs -r -P"$(nproc)" -n2 sh -c 'case "$0" in *.c) gcc -O2 -w '"${INC[*]}"' -c "$0" -o "$1";; *) g++ '"${FLAGS[*]}"' -c "$0" -o "$1";; esac'

g++ "${FLAGS[@]}" -include tools/capture/shim/shim.h tools/capture/capture.cpp "$OBJ"/*.o $(sdl2-config --libs) -lpthread -o .pio/capture/capture

FR=.pio/capture/frames
rm -rf "$FR"; .pio/capture/capture "$FR" "$@"
mkdir -p previews
for f in "$FR"/*.raw; do
  k=$(basename "$f" .raw)
  ffmpeg -loglevel error -y -f rawvideo -pix_fmt rgb8 -s 320x240 -framerate 15 -i "$f" \
    -vf "split[a][b];[a]palettegen=max_colors=128:stats_mode=diff[p];[b][p]paletteuse=dither=none:diff_mode=rectangle" \
    "previews/$k.gif"
done
du -ch previews/*.gif | tail -1
