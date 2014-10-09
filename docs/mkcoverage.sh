#!/bin/bash
# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
# Author: 2014, Tim Janik, see https://testbit.eu/

SCRIPTNAME="$0"
function die  { e="$1"; shift; [ -n "$*" ] && echo "$SCRIPTNAME: $*" >&2; exit "$e" ; }
ORIGDIR=`pwd`

# exit on errors
set -e

# parse options
TREE_ONLY=false
while [[ $# > 1 ]] ; do case "$1" in
  --tree-only)	TREE_ONLY=true ;;
  --)		shift ; break ;;
  *)		break ;;
esac ; shift ; done

# echo commands
test "$V" != "1" || set -x

# find dist tarball
TARBALL="$1"
test -n "$TARBALL" -a -e "$TARBALL" || die 1 "missing tarball"

# extract and build in tarball directory
TARBALLF=`basename "$TARBALL"`
TARDIR="${TARBALLF/.tar*/}"
BUILDDIR="coverage-tree/$TARDIR"
test -e "$BUILDDIR" || {
  mkdir -p coverage-tree
  tar -x -C coverage-tree/ -f "$TARBALL" "$TARDIR"
  # build package with coverage enabled
  ( cd "$BUILDDIR"
    ./configure CC="gcc --coverage" CXX="g++ --coverage" --prefix=`pwd`/inst
    NCPUS=`fgrep processor /proc/cpuinfo | wc -l`
    nice make -j$NCPUS
    make install
  )
}
! $TREE_ONLY || exit 0

# create coverage reports in BUILDDIR
cd "$BUILDDIR"

# remove old coverage data (only needed if not using a fresh build)
find . -name '*.gcda' -exec rm '{}' \;

# create coverage baseline
lcov -c -d . -o cov-base.lcov -i

# execute code to generate coverage
make check installcheck

# create test coverage profile
lcov -c -d . -o cov-tests.lcov

# merge coverage profiles
lcov -a cov-base.lcov -a cov-tests.lcov -o cov-stage1.lcov

# strip system libs and extra files
lcov -r cov-stage1.lcov '/usr/*' -o cov-stage2.lcov
lcov -r cov-stage2.lcov "`pwd`/tmpx.cc" -o cov-stage3.lcov
lcov -r cov-stage3.lcov "`pwd`/inst/*" -o Coverage-Report

# generate coverage report as HTML
genhtml -o coverage/ --demangle-cpp --no-branch-coverage Coverage-Report -t 'check installcheck'

# package report
test ! -e "$ORIGDIR/coverage/" || rm -r "$ORIGDIR/coverage/"
mv -v coverage/ "$ORIGDIR"
