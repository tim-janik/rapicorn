#!/bin/bash
# CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
# Author: 2014, Tim Janik, see https://testbit.eu/

# exit on errors
set -e
# echo commands
test "$V" != "1" || set -x

ORIGDIR=`pwd`

# find dist tarball
TARBALL="$1"
test -n "$TARBALL" -a -e "$TARBALL" || { echo "$0: missing tarball" ; false; }

# extract and build in temporary directory
TARBALLF=`basename "$TARBALL"`
BUILDDIR="coverage-build/${TARBALLF/.tar*/}"
test -e "$BUILDDIR" || {
  mkdir -p coverage-build
  tar -x -C coverage-build/ -f "$TARBALL"
  # build package with coverage enabled
  ( cd "$BUILDDIR"
    ./configure CC="gcc --coverage" CXX="g++ --coverage" --prefix=`pwd`/inst
    NCPUS=`fgrep processor /proc/cpuinfo | wc -l`
    nice make -j$NCPUS
    make install
  )
}
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

# cleanup
#cd "$ORIGDIR"
#rm -rf coverage-build/
