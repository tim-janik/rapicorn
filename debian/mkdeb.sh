#!/bin/bash
# This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0

# Build Debian package from debiandir, tarball and revision

echo "$@"
set -ex

SCRIPTNAME=`basename $0`
function die  { e="$1"; shift; [ -n "$*" ] && echo "$SCRIPTNAME: $*" >&2; exit "$e" ; }

test "$#" -ge 2 -a "$#" -le 4 || die 11 "Usage: $0 debiandir/ pathto/upstream-version.tar.xz revision [message]"
ARG_DEBIANDIR="$1" ; ARG_TARBALL="$2" ; ARG_REVISION="${3:-rev00}" ; ARG_MESSAGE="$4"

# Construct package configuration
TARBALLNAME=`basename $ARG_TARBALL`
UPSTREAMTARDIR=${TARBALLNAME%.tar*}
UPSTREAMVERSION=${UPSTREAMTARDIR#*-}
PACKAGE=`dpkg-parsechangelog -l$ARG_DEBIANDIR/changelog --show-field Source`
PACKAGEDIR=$PACKAGE-$UPSTREAMVERSION
DEBVERSION="$UPSTREAMVERSION-$ARG_REVISION"
DEBTARBALL=$PACKAGE'_'$DEBVERSION.orig.tar.xz
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
# Log to debian/changelog
( cd $PACKAGEDIR/
  dch -v "$DEBVERSION-1" "${ARG_MESSAGE:-Build $DEBVERSION}"
  dch -r "" -D experimental
)
cat $PACKAGEDIR/debian/changelog

# Build source package
dpkg-source -b $PACKAGEDIR/

# Build binary package
#( cd $PACKAGEDIR/ && debuild -uc -tc -rfakeroot )

# Build pristine binary package
( cd $PACKAGEDIR/ && sudo pdebuild --buildresult ./.. --debbuildopts -j$(nproc) )
