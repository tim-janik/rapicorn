#!/bin/sh
set -e

# "Usage: reslist.sh"
# Update and display res/resfiles.list

SCRIPTNAME="$(basename "$0")" ; die() { e="$1"; shift; echo "$SCRIPTNAME: $*" >&2; exit "$e"; }
SCRIPTDIR="$(dirname "$(readlink -f "$0")")"

cd "$SCRIPTDIR/../res/"

RESFILES_LIST="$(cat resfiles.list)"
FILE_LIST="$(find -type f |
	     grep -Ev '/Makefile[^/]*$|/GNUmakefile|/meson.build|/resfiles.list$' |
             sed 's,^[.]/,,' |
             sort)"

if [ "$FILE_LIST" != "$RESFILES_LIST" ] ; then
    echo "  UPDATE   res/resfiles.list" >&2
    echo "$FILE_LIST" >resfiles.list
fi

cat resfiles.list
