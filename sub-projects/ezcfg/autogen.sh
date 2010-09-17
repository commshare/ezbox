#!/bin/sh -e

autoreconf --install --symlink --force

MYCFLAGS="-g -Wall \
-Wmissing-declarations -Wmissing-prototypes \
-Wnested-externs -Wpointer-arith \
-Wpointer-arith -Wsign-compare -Wchar-subscripts \
-Wstrict-prototypes -Wshadow \
-Wformat=2 -Wtype-limits"

case "$CFLAGS" in
	*-O[0-9]*)
		;;
	*)
		MYCFLAGS="$MYCFLAGS -O2"
		;;
esac

libdir() {
	echo $(cd $1/$(gcc -print-multi-os-directory); pwd)
}

args="--prefix=/usr \
--sysconfdir=/etc \
--sbindir=/sbin \
--libdir=$(libdir /usr/lib) \
--libexecdir=/lib/ezcfg"

export CFLAGS="$CFLAGS $MYCFLAGS"
./configure $args $@
