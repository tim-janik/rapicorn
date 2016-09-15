#!/bin/sh
set -e

# Usage: mkbuildid.sh
# Update BuildID and display source file name

SCRIPTNAME="$(basename "$0")" ; die() { e="$1"; shift; echo "$SCRIPTNAME: $*" >&2; exit "$e"; }
SCRIPTDIR="$(dirname "$(readlink -f "$0")")"

test "$#" -ge 1 || die 1 "Usage: mkbuildid.sh <fallbackversion> [outfile] [depfile]"
FALLBACK_VERSION="$1"
BUILDID_CC="$2"
BUILDID_D="$3"
GITINDEX="${SCRIPTDIR}/../.git/index"

if test -e "$GITINDEX" ; then			# from .git
  BUILDID=$(
    git describe --long --match '[0-9]*.*[0-9]' --first-parent --abbrev=14 HEAD |
	sed 's/-\([^-]*-g\)/+r\1/'
	 )
else						# without .git
  BUILDID="${FALLBACK_VERSION-0.0.0}-untracked"
fi

BUILDID_CODE=$(cat <<__EOF
#include "rcore/cxxaux.hh" // const char* buildid();
namespace RapicornInternal {
  const char* buildid() { return "$BUILDID"; }
}
__EOF
)

if test -z "$BUILDID_CC" ; then
  echo "$BUILDID"
elif test ! -e "$BUILDID_CC" || test "$(cat "$BUILDID_CC")" != "$BUILDID_CODE" ; then
  echo "$BUILDID_CODE" > "$BUILDID_CC"
fi

test -z "$BUILDID_D" -o ! -e "$GITINDEX" ||
    echo "$BUILDID_CC: $GITINDEX"		 >"$BUILDID_D"
