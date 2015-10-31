#!/bin/bash
# This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0

# Build Debian package from debiandir, tarball and revision

echo "$@"
set -ex

TMPDIR=`mktemp -d "/tmp/mkdeb$$-XXXXXX"` &&
  trap 'rm -rf "$TMPDIR"' 0 HUP QUIT TRAP USR1 PIPE TERM

SCRIPTNAME=`basename $0`
function die  { e="$1"; shift; [ -n "$*" ] && echo "$SCRIPTNAME: $*" >&2; exit "$e" ; }

test "$#" -ge 2 -a "$#" -le 4 || die 11 "Usage: $0 debiandir/ pathto/upstream-version.tar.xz revision [release_message]"
ARG_DEBIANDIR="$1" ; ARG_TARBALL="$2" ; ARG_REVISION="${3:--00unrevisioned}" ; ARG_MESSAGE="$4"

# Construct package configuration
TARBALLNAME=`basename $ARG_TARBALL`
UPSTREAMTARDIR=${TARBALLNAME%.tar*}
UPSTREAMVERSION=${UPSTREAMTARDIR#*-}
PACKAGE=`dpkg-parsechangelog -l$ARG_DEBIANDIR/changelog --show-field Source`
PACKAGEDIR=$PACKAGE-$UPSTREAMVERSION
DEBVERSION="$UPSTREAMVERSION$ARG_REVISION"
DEBTARBALL=$PACKAGE'_'${DEBVERSION%-*}.orig.tar.xz
cat <<__EOF
ARG_DEBIANDIR=$ARG_DEBIANDIR
ARG_TARBALL=$ARG_TARBALL
ARG_REVISION=$ARG_REVISION
ARG_MESSAGE=$ARG_MESSAGE
TARBALLNAME=$TARBALLNAME
UPSTREAMTARDIR=$UPSTREAMTARDIR
UPSTREAMVERSION=$UPSTREAMVERSION
PACKAGE=$PACKAGE
PACKAGEDIR=$PACKAGEDIR
DEBTARBALL=$DEBTARBALL
__EOF

# Setup source package
rm -rf $PACKAGEDIR $UPSTREAMTARDIR
# Rename the upstream tarball: <PACKAGE>_<VERSION>.orig.tar.xz
cp $ARG_TARBALL $DEBTARBALL
# Unpack the upstream tarball
tar xf $DEBTARBALL # -> $UPSTREAMTARDIR/
test "$UPSTREAMTARDIR" = "$PACKAGEDIR" || mv $UPSTREAMTARDIR $PACKAGEDIR
# Add Debian files: debian/
cp -a $ARG_DEBIANDIR $PACKAGEDIR/debian
# is this a maintainer build?
MAINTAINER_BUILD=true
pushd $PACKAGEDIR/
dpkg-parsechangelog --show-field Maintainer | fgrep -q "<$EMAIL>" || MAINTAINER_BUILD=false
popd
# Log to debian/changelog
( cd $PACKAGEDIR/
  if test -z "$ARG_MESSAGE" ; then
    dch -v "$DEBVERSION" "Build $DEBVERSION"
  else
    dch -v "$DEBVERSION" "$ARG_MESSAGE"
    dch -r "" # -D experimental
  fi
)
cat $PACKAGEDIR/debian/changelog

# Build source package
dpkg-source -b $PACKAGEDIR/

# Build binary package, using pbuilder if requested
if test "$USE_PBUILDER" = true ; then
  mkdir $TMPDIR/phooks
  cat > $TMPDIR/phooks/B90lintian <<-__EOF
	#!/bin/bash
	set -e
	test -x /usr/bin/lintian ||
	  apt-get -y "${APTGETOPT[@]}" install lintian
	echo "Running lintian..."
	su -c "lintian -I --show-overrides /tmp/buildd/*.changes; :" - pbuilder
	__EOF
  chmod +x $TMPDIR/phooks/B90lintian
  ( cd $PACKAGEDIR/ && sudo pdebuild --buildresult ./.. --debbuildopts -j$(nproc) -- --hookdir $TMPDIR/phooks )
else
  ( cd $PACKAGEDIR/
    unset ENABLE_CCACHE NOSIGN
    # enable ccache if possible
    test -d /usr/lib/ccache/ && ENABLE_CCACHE='--prepend-path=/usr/lib/ccache/ -eCCACHE_*'
    # skip signing for non-maintainers
    $MAINTAINER_BUILD || NOSIGN='-us -uc'
    # build with debuild which passes options to dpkg-buildpackage
    debuild $ENABLE_CCACHE -rfakeroot -j$(nproc) $NOSIGN
  )
fi
# Build package index for apt
dpkg-scanpackages . > Packages	# apt-ftparchive packages . > Packages
# echo "deb [trusted=yes] file:///"`pwd`" ./" > /etc/apt/sources.list.d/localfiles.list
