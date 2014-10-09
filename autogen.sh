#!/bin/bash

CONFIGURE_OPTIONS=--enable-devel-mode=yes

echo "$0: Cleaning configure caches..."
rm -rf autom4te.cache/
rm -f  config.cache

echo "$0: Enforce ChangeLog presence"	# automake *requires* ChangeLog
test -e ChangeLog || TZ=GMT0 touch ChangeLog -t 190112132145.52

echo "$0: autoreconf -vfsi -Wno-portability"
autoreconf -vfsi -Wno-portability || exit $?

echo "$0: Override po/Makefile.in.in with po/Makefile.intltool"
rm -f po/Makefile.in.in && cp -v po/Makefile.intltool po/Makefile.in.in || exit $?

echo "$0: ./configure $CONFIGURE_OPTIONS $@"
./configure $CONFIGURE_OPTIONS "$@" || exit $?
