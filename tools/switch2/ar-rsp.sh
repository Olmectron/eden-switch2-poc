#!/bin/sh
# Archiver wrapper for MSYS2 hosts: devkitA64's tools are native Windows programs,
# whose command line is limited to 32k characters, and libcore.a has more objects
# than that. Passes the member list through a response file instead.
#
# Usage: ar-rsp.sh <operation> <archive> <members...>

: "${DEVKITPRO:=/opt/devkitpro}"
AR="$DEVKITPRO/devkitA64/bin/aarch64-none-elf-gcc-ar"

if [ $# -le 2 ]; then
    exec "$AR" "$@"
fi

op=$1
archive=$2
shift 2

rsp="$archive.rsp"
: > "$rsp"
for member in "$@"; do
    printf '"%s"\n' "$member" >> "$rsp"
done

"$AR" "$op" "$archive" "@$rsp"
status=$?
rm -f "$rsp"
exit $status
