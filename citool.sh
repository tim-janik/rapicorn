#!/bin/bash

set -e

# die with descriptive error messages
STARTPWD=`pwd`; SCRIPTNAME=`basename $0` ;
function die { e="$1"; [[ $e =~ ^[0-9]+$ ]] && shift || e=127; echo "$SCRIPTNAME: fatal: $*" >&2; exit "$e" ; }

# == config ==
mkconfig() {
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
  DCHMESSAGE="Automatic CI snapshot, git commit $COMMITID"
  echo DCHMESSAGE="\"$DCHMESSAGE\""
}

# == pcreate [distribution] ==
pcreate() {
  set -x
  CIDIR=`pwd`/cidir/ ; mkdir -p "$CIDIR"			########## cidir ##########
  DIST="$1"; SOURCES="$2"
  test -n "$1" || die "missing tarball"
  sudo pbuilder create --distribution $DIST --debootstrapopts --variant=buildd
  cat > $CIDIR/pcreate-cmds <<-__EOF
	apt-get -y install apt-transport-https ca-certificates &&
	  { ! fgrep -q 'NAME="Ubuntu"' /etc/os-release ||
	      sed "/^deb /s/ main\b/ universe main/" -i /etc/apt/sources.list ; } &&
	  echo "$SOURCES" | tee /etc/apt/sources.list.d/extra-sources.list &&
	  apt-get update
	__EOF
  sudo pbuilder --login --save-after-login < $CIDIR/pcreate-cmds
  rm -f $CIDIR/pcreate-cmds
}

# == pbuild <TARBALL> <DEBIANDIR/> ==
pbuild() {
  set -x
  CIDIR=`pwd`/cidir/ ; mkdir -p "$CIDIR"			########## cidir ##########
  test -n "$1" || die "missing tarball"
  test -n "$2" || die "missing debian/"
  mkconfig # PACKAGE, VERSION, ...
  DEBVERSION="$VERSION$VCSREVISION"
  # setup lintian for pbuilder
  mkdir $CIDIR/.hooks
  cat > $CIDIR/.hooks/B90lintian <<-__EOF
	#!/bin/bash
	set -e
	test -x /usr/bin/lintian || apt-get -y install lintian
	echo "Running lintian..."
	su -c "lintian -I --show-overrides /tmp/buildd/*.changes; :" - pbuilder
	__EOF
  chmod +x $CIDIR/.hooks/B90lintian
  # copy and extract tarball
  mkdir $CIDIR/.xdest
  TARBALL="$PACKAGE""_${DEBVERSION%-*}.orig.tar.${1##*.tar.}" # $1
  cp "$1" $CIDIR/$TARBALL
  # extract tarball and rename content directory
  tar -C $CIDIR/.xdest/ -xf $CIDIR/$TARBALL
  TARDIR=`ls $CIDIR/.xdest/` && test 1 = `echo $TARDIR | wc -l` &&
    test -d $CIDIR/.xdest/$TARDIR || die "tarball must contain a single directory"
  mv "$CIDIR/.xdest/$TARDIR" "$CIDIR/$PACKAGE-$VERSION"
  TARDIR="$PACKAGE-$VERSION"
  rmdir $CIDIR/.xdest
  # copy debian/ into place
  cp -a "$2/" "$CIDIR/$TARDIR/debian"
  cd "$CIDIR/$TARDIR/"; pwd					########## cd ##########
  DEBPACKAGE=`dpkg-parsechangelog --show-field Source`
  test -n "$DEBVERSION" || DEBVERSION=`dpkg-parsechangelog --show-field Version`
  DEBARCH="${ARCHITECTURE:-$(dpkg-architecture -qDEB_HOST_ARCH)}"
  test "$PACKAGE" = "$DEBPACKAGE" || die "upstream vs debian package name mismatch: $PACKAGE == $DEBPACKAGE"
  # update changelog
  dch -v "$DEBVERSION" "$DCHMESSAGE"
  dch -r "" # release build
  cat debian/changelog
  # build source package
  dpkg-source -b .
  # pbuilder debuild
  pdebuild --buildresult ./.. --debbuildopts -j`nproc` -- --hookdir $CIDIR/.hooks
  cd $CIDIR 							########## cd ##########
  rm -rf "$CIDIR/.hooks/" "$CIDIR/$TARDIR/"
  pwd
  ls -al
}

# == pdist ==
pdist() {
  # $@ contains required package list
  mkconfig # PACKAGE, VERSION, ...
  set -x
  CIDIR=`pwd`/cidir/ ; mkdir -p "$CIDIR"			########## cidir ##########
  exec > >(tee "cidir/citool-pdist.log") 2>&1
  # pbuilder scripts, note the difference between $@ and \$@
  cat > cidir/pdist-cmds-root <<-__EOF
	#!/bin/bash
	set -ex
	export LOGNAME=root
	STARTWD=\$PWD
	cd "\$1"
	groupadd -g \$3 -o ciuser
	useradd -g ciuser -u \$2 -d /tmp -o ciuser
	apt-get -y install $@
	echo cidir/pdist-cmds-user "\$@" | su -p ciuser
	__EOF
  MAKE="make -j`nproc` V=1"
  cat > cidir/pdist-cmds-user <<-__EOF
	#!/bin/bash
	( set -ex
	  # configure with user writable prefix
	  test ! -x ./autogen.sh ||
	    ./autogen.sh --prefix=`pwd`/_pdist_inst
	  test -e ./Makefile ||
	    ./configure --prefix=`pwd`/_pdist_inst
	  # figure the name of the dist tarball
	  ( # sed extracts all Makefile variable assignments (first line suffices)
	    sed 's/\(^[	 ]*[0-9a-z_A-Z]\+[ 	]*:\?=.*\)\|\(.*\)/\1/' Makefile
	    echo 'all: ; @echo \$(firstword \$(DIST_ARCHIVES))' ) > cidir/pdist-tmk
	  DIST_TARBALL=\$(make -f cidir/pdist-tmk) || exit \$?
	  echo DIST_TARBALL=\$DIST_TARBALL
	  test -n "\$DIST_TARBALL"
	  rm -f cidir/pdist-tmk
	  # build, check and dist
	  $MAKE
	  $MAKE check
	  $MAKE install
	  $MAKE installcheck
	  $MAKE dist
	  $MAKE uninstall
	  $MAKE clean
	  set -e
	  ls -l \$DIST_TARBALL
	  mv -v \$DIST_TARBALL cidir/
	) ; EX=\$?
	# test 0 = \$EX || /bin/bash < /dev/tty > /dev/tty 2> /dev/tty
	exit \$EX
	__EOF
  # pbuilder executes cmds-root which executes cmds-user
  chmod +x cidir/pdist-cmds-user cidir/pdist-cmds-root
  sudo pbuilder --execute --bindmounts $(readlink -f ./) -- \
    cidir/pdist-cmds-root $PWD $(id -u) $(id -g)
  rm -f cidir/pdist-cmds-user cidir/pdist-cmds-root cidir/citool-pdist.log
  # cmds-user builds dist tarballs and moves those to cidir/
}

# == bintrayup ==
bintrayup() {
  set +x # avoid printing authentication secrets
  mkconfig >/dev/null # PACKAGE, VERSION, ...
  ACCNAME="$1"; PKGDIST="$2"; PKGPATH="$3" # BINTRAY_APITOKEN set by caller
  test -n "$ACCNAME" || die "missing bintray account"
  test -n "$PKGDIST" || die "missing distribution"
  test -n "$PKGPATH" || die "missing package path"
  shift 3
  # create new bintray versoin
  REPOVERSION="CI-git$TOTAL_COMMITS" # echo "REPOVERSION=$REPOVERSION"
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
[[ "$1" != pcreate ]] 	|| { shift; pcreate "$@" ; exit $? ; }
[[ "$1" != pbuild ]]	|| { shift; pbuild "$@" ; exit $? ; }
[[ "$1" != pdist ]]	|| { shift; pdist "$@" ; exit $? ; }
[[ "$1" != bintrayup ]]	|| { shift; bintrayup "$@" ; exit $? ; }
