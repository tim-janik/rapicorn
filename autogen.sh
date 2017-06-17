#!/bin/bash

set -xe # be verbose and abort on errors

rm -rf autom4te.cache/ ./config.cache ./config.status

if false ; then # the autoreconf overhead almost doubles autotools runtime
  autoreconf --verbose --force --symlink --install
else
  ${ACLOCAL:-aclocal}		--force				# --verbose
  ${LIBTOOLIZE:-libtoolize}	--force --install		# --verbose
  ${AUTOCONF:-autoconf}		--force				# --verbose
  ${AUTOHEADER:-autoheader}	--force				# --verbose
  ${AUTOMAKE:-automake}		--add-missing --force-missing	# --verbose
fi

intltoolize --force --automake # intltool-extract.in intltool-merge.in intltool-update.in po/Makefile.in.in
rm -f po/Makefile.in.in && cp -v po/Makefile.intltool po/Makefile.in.in # fix po/Makefile...

test -n "$NOCONFIGURE" || "./configure" "$@"
