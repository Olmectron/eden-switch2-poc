#!/bin/sh -e
# Builds targets of the Switch 2 configuration (see configure.sh).
# The devkitA64 tools must be on PATH: bundled OpenSSL calls them by name.
#
# Usage: tools/switch2/build.sh [ninja args], e.g. build.sh -k 0 common dynarmic

: "${DEVKITPRO:?Set DEVKITPRO (e.g. /opt/devkitpro)}"
. "$DEVKITPRO/switch2vars.sh"

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
exec ninja -C "$ROOT/build/switch2" "$@"
