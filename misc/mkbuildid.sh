#!/bin/bash
set -e

# Usage: mkbuildid.sh [outfile] [depfile]
# Update BuildID and display source file name

SCRIPTNAME="$(basename "$0")" ; die() { e="$1"; shift; echo "$SCRIPTNAME: $*" >&2; exit "$e"; }
SCRIPTDIR="$(dirname "$(readlink -f "$0")")"

# Extract version, see: https://www.debian.org/doc/debian-policy/ch-controlfields.html#s-f-Version
FALLBACK_VERSION="$(sed -nr "/^AC_INIT\b/{ s/^[^,]*,[^0-9]*([A-Za-z0-9.:~+-]*).*/\1/; p; }" $SCRIPTDIR/../configure.ac)"
test -n "$FALLBACK_VERSION" || die 7 "failed to detect AC_INIT in $SCRIPTDIR/../configure.ac"

BUILDID_CC="$1"
BUILDID_D="$2"
DOTGIT=`git rev-parse --git-dir 2>/dev/null` || true

gen_buildid() {
  test -e "$DOTGIT" ||				# not in a git-tracked source tree
      { printf %s "${FALLBACK_VERSION-0.0.0}-untracked" ; return ; }
  COMMITID="${1-HEAD}"
  DESC=$(git describe --match '[0-9]*.*[0-9]' --abbrev=5 $COMMITID)
  test "$DESC" != "${DESC%%-*}" ||		# HEAD is on release tag
      { echo "$DESC" ; return ; }
  # HEAD has commits on top of last release tag, transform 3.2.1-7-gabc -> 3.2.2~7-gabc
  MICRO=$(printf %s "$DESC" | sed 's/^[^-]*\b\([0-9]\+\)-.*/\1/')
  [[ "$MICRO" =~ ^[0-9]+$ ]] || die 7 "failed to detect MICRO in $DESC"
  MICRO=$(expr 1 + "$MICRO")
  NEXT=$(printf %s "$DESC" | sed "s/^\([^-]*\)\b\([0-9]\+\)-/\1$MICRO~wip/")
  printf %s "$NEXT"
}

BUILDID=$(gen_buildid HEAD)

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

test -z "$BUILDID_D" -o ! -e "$DOTGIT" || {
  GITINDEX=`git rev-parse --show-toplevel`/`git rev-parse --git-path index`
  echo "$BUILDID_CC: $GITINDEX"		 >"$BUILDID_D"
}
