#!/bin/sh
BUILD_DIR="./build"
if test -f "$BUILD_DIR/build.ninja"
then
	echo -n
else
	meson "$BUILD_DIR" || exit "$?"
fi
ninja -C "$BUILD_DIR" || exit "$?"
exec lighttpd -D -f "$BUILD_DIR/lighttpd.conf"
