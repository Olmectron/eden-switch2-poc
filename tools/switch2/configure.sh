#!/bin/sh -e
# Configures Eden as a Switch 2 homebrew (libounce + OunceVK + kaiHEN JIT).
# Proof of concept: core libraries only, no frontend yet.
#
# Needs devkitPro with the ounce-devkit packages (libounce, ouncevk, libkaihen):
#   https://github.com/ouncebrew/ounce-devkit
# and these devkitPro portlibs:
#   switch-libzstd switch-libopus switch-enet switch-ffmpeg switch-lz4
#
# Usage: tools/switch2/configure.sh [extra cmake args]

: "${DEVKITPRO:?Set DEVKITPRO (e.g. /opt/devkitpro)}"
. "$DEVKITPRO/switch2vars.sh"

# Host tool for shader compilation. On MSYS2: mingw-w64-ucrt-x86_64-glslang.
GLSLANG=$(command -v glslangValidator || command -v /ucrt64/bin/glslangValidator || true)
: "${GLSLANG:?glslangValidator not found}"
case "$(uname -s)" in MSYS*|MINGW*) GLSLANG=$(cygpath -m "$GLSLANG") ;; esac
set -- -DGLSLANGVALIDATOR="$GLSLANG" "$@"

ROOT=$(cd "$(dirname "$0")/../.." && pwd)

cmake -S "$ROOT" -B "$ROOT/build/switch2" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/libounce/cmake/Switch2.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCPMUTIL_FORCE_BUNDLED=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DYUZU_STATIC_BUILD=ON \
    -DENABLE_QT=OFF \
    -DYUZU_CMD=OFF \
    -DYUZU_ROOM=OFF \
    -DYUZU_ROOM_STANDALONE=OFF \
    -DYUZU_TESTS=OFF \
    -DBUILD_TESTING=OFF \
    -DENABLE_OPENGL=OFF \
    -DENABLE_CUBEB=OFF \
    -DENABLE_LIBUSB=OFF \
    -DENABLE_WEB_SERVICE=OFF \
    -DENABLE_UPDATE_CHECKER=OFF \
    -DENABLE_RESHADE=OFF \
    -DENABLE_WERROR=OFF \
    -DYUZU_CRASH_DUMPS=OFF \
    -DUSE_DISCORD_PRESENCE=OFF \
    -Dzstd_FORCE_SYSTEM=ON \
    -DOpus_FORCE_SYSTEM=ON \
    -Denet_FORCE_SYSTEM=ON \
    -DFFmpeg_FORCE_SYSTEM=ON \
    "$@"
