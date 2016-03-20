#!/bin/bash
# Copyright (C) 2015 Tim Janik / MPL-2.0

set -e # abort on errors

# die with descriptive error messages
STARTPWD=`pwd`; SCRIPTNAME=`basename $0` ;
function die { e="$1"; [[ $e =~ ^[0-9]+$ ]] && shift || e=127; echo "$SCRIPTNAME: fatal: $*" >&2; exit "$e" ; }

# == config ==
mkconfig() {
  pushd $(dirname $(readlink -f $0)) >/dev/null 	# cd rapicorn/
  # extract AC_INIT package and version
  test -z "$PACKAGE" &&
    PACKAGE=`sed -nr "/^AC_INIT\b/{ s/^[^(]*\([ 	]*([^,	 ]+).*/\1/; s/\[|]//g; p; }" configure.ac`
  echo PACKAGE=$PACKAGE
  test -z "$VERSION" &&
    VERSION=`sed -nr "/^AC_INIT\b/{ s/^[^,]*,[^0-9]*([0-9.]*).*/\1/; p; }" configure.ac`
  [[ $VERSION =~ ^[0-9.]+$ ]] || die "failed to detect package version"
  echo VERSION=$VERSION
  test ! -s `git rev-parse --git-dir`/shallow || die "shallow git repository encountered"
  TOTAL_COMMITS=`git rev-list --count HEAD` # count commits to provide a monotonically increasing revision
  echo TOTAL_COMMITS=$TOTAL_COMMITS
  VCSREVISION="+git$TOTAL_COMMITS-0ci${CITOOL_BUILDID:-${TRAVIS_JOB_NUMBER:-1}}"
  echo VCSREVISION=$VCSREVISION
  PUBVERSION="CI-git$TOTAL_COMMITS"
  echo PUBVERSION=$PUBVERSION
  COMMITID=`git rev-parse HEAD`
  echo COMMITID=$COMMITID
  CHANGELOGMSG="Automatic CI snapshot, git commit $COMMITID"
  echo CHANGELOGMSG="\"$CHANGELOGMSG\""
  popd >/dev/null					# cd OLDPWD
}


# == bintrayup ==
bintrayup() {
  set +x # avoid printing authentication secrets
  mkconfig >/dev/null # PACKAGE, VERSION, ...
  ACCNAME="$1"; PKGPATH="$2"; PKGDIST="$3" # BINTRAY_APITOKEN must be set by caller
  test -n "$ACCNAME" || die "missing bintray account"
  test -n "$PKGPATH" || die "missing package path"
  test -n "$PKGDIST" || die "missing distribution"
  shift 3
  # create new bintray versoin
  REPOVERSION="$VERSION+git$TOTAL_COMMITS" # echo "REPOVERSION=$REPOVERSION"
  echo "  REMOTE  " "creating new version: $REPOVERSION"
  curl -d "{ \"name\": \"$REPOVERSION\", \"released\": \"`date -I`\", \"desc\": \"Automatic CI Build\" }" \
    -u"$ACCNAME:$BINTRAY_APITOKEN" "https://api.bintray.com/packages/$ACCNAME/$PKGPATH/versions" \
    -H"Content-Type: application/json" -f && EX=$? || EX=$?
  test $EX = 0 -o $EX = 22 # 22 indicates HTTP responses >= 400, the version likely already exists
  # upload individual files
  # NOTE: we cannot use "explode=1" b/c files may have different architectures, which
  # need to be passed corrctly in the upload to regenerate the Packages index.
  URL="https://api.bintray.com/content/$ACCNAME/$PKGPATH/$REPOVERSION"
  OPTS="deb_distribution=$PKGDIST;deb_component=main;explode=0;override=0;publish=1"
  ALLOK=0
  for F in "$@" ; do
    S="${F%.deb}"; A="${S##*_}"
    test ! -z "$A" || continue
    echo "  REMOTE  " "uploading: $F ($A)"
    curl -T "$F" -u"$ACCNAME:$BINTRAY_APITOKEN" "$URL/`basename $F`;$OPTS;deb_architecture=$A" -f && EX=$? || EX=$?
    ALLOK=$(($ALLOK + $EX))
  done
  test $ALLOK = 0 || die "Some files failed to upload"
}

# == commands ==
[[ "$1" != config ]]	|| { shift; mkconfig "$@" ; exit $? ; }
[[ "$1" != bintrayup ]]	|| { shift; bintrayup "$@" ; exit $? ; }
