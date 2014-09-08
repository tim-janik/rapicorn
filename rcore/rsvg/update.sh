#!/bin/bash
# Rapicorn - Copyright (C) 2011 Tim Janik
# This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0

function die { e="$1"; shift; [ -n "$*" ] && echo "$0: $*"; exit "$e" ; }

set -e # exit on error
set -x # expanded cmd display

# ensure temporary dir
mkdir -p .upstream/
cd .upstream/

# pull git updates
GITURL=git://git.gnome.org/librsvg
[ -e .git/ ] || git clone $GITURL
git pull origin master

# extract library versioning needed for generated configuration files
HC="`git rev-parse --verify HEAD   2>/dev/null`";
LIBRSVG_MAJOR_VERSION=`sed -n '/^\s*m4_define.*\[rsvg_major_version\]/{ s/^.*\[\([0-9]\+\).*/\1/; p }' < configure.in`
LIBRSVG_MINOR_VERSION=`sed -n '/^\s*m4_define.*\[rsvg_minor_version\]/{ s/^.*\[\([0-9]\+\).*/\1/; p }' < configure.in`
LIBRSVG_MICRO_VERSION=`sed -n '/^\s*m4_define.*\[rsvg_micro_version\]/{ s/^.*\[\([0-9]\+\).*/\1/; p }' < configure.in`
[ -z "$HC" -o -z "$LIBRSVG_MAJOR_VERSION$LIBRSVG_MINOR_VERSION$LIBRSVG_MICRO_VERSION" ] &&
	die 1 "Failed to find and parse librsvg version information"

# create generated files
cat <<-__EOF >../config.h
	// librsvg $LIBRSVG_MAJOR_VERSION.$LIBRSVG_MINOR_VERSION.$LIBRSVG_MICRO_VERSION
	// $GITURL HEAD=$HC
	#include "configure.h"
__EOF
sed "	s/@LIBRSVG_MAJOR_VERSION@/$LIBRSVG_MAJOR_VERSION/g ;
	s/@LIBRSVG_MINOR_VERSION@/$LIBRSVG_MINOR_VERSION/g ;
	s/@LIBRSVG_MICRO_VERSION@/$LIBRSVG_MICRO_VERSION/g ;
	s/@PACKAGE_VERSION@/$LIBRSVG_MAJOR_VERSION.$LIBRSVG_MINOR_VERSION.$LIBRSVG_MICRO_VERSION/g ;" \
    < librsvg-features.h.in > ../librsvg-features.h

# update source files
cp \
        rsvg-affine.c librsvg-features.c rsvg-bpath-util.c rsvg-bpath-util.h rsvg-css.c rsvg-css.h rsvg-defs.c rsvg-defs.h \
        rsvg-image.c rsvg-image.h rsvg-paint-server.c rsvg-paint-server.h rsvg-path.c rsvg-path.h rsvg-private.h \
        rsvg-base-file-util.c rsvg-filter.c rsvg-filter.h rsvg-marker.c rsvg-marker.h rsvg-mask.c rsvg-mask.h \
        rsvg-shapes.c rsvg-shapes.h rsvg-structure.c rsvg-structure.h rsvg-styles.c rsvg-styles.h rsvg-text.c rsvg-text.h \
        rsvg-cairo-draw.c rsvg-cairo-draw.h rsvg-cairo-render.c rsvg-cairo-render.h rsvg-cairo-clip.h rsvg-cairo-clip.c \
        rsvg-cond.c rsvg-base.c rsvg.c rsvg-gobject.c rsvg-file-util.c rsvg-xml.c rsvg-xml.h \
  	rsvg.h rsvg-cairo.h \
  ../

# patch source files
UUID=`uuidgen | sed s/-//g`
cat <<-__EOF | patch -p0
	--- ../rsvg-gobject.c
	+++ ../rsvg-gobject.c
	@@ -328,1 +328,1 @@
	-                                             "RsvgHandle", &rsvg_type_info, (GTypeFlags) 0);
	+                                             "Rapicorn_RsvgHandle_$UUID", &rsvg_type_info, (GTypeFlags) 0);
	@@ -337,1 +337,1 @@ rsvg_handle_get_type (void)
	-            g_type_register_static (G_TYPE_OBJECT, "RsvgHandle", &rsvg_type_info, (GTypeFlags) 0);
	+            g_type_register_static (G_TYPE_OBJECT, "Rapicorn_RsvgHandle_$UUID", &rsvg_type_info, (GTypeFlags) 0);
__EOF
