# Rapicorn
# Copyright (C) 2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = . birnet rapicorn po examples tests

# setup automake
AUTOMAKE_OPTIONS = 1.9 dist-bzip2 no-dist-gzip

test:
	$(MAKE) -C tests/ $(AM_MAKEFLAGS) $@
.PHONY: test

# extra dependancies
configure: birnet/acbirnet.m4 birnet/configure.inc


CLEANFILES += intltool-extract intltool-merge intltool-update

EXTRA_DIST += intltool-extract.in intltool-merge.in intltool-update.in

# install pkg-config files
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rapicorn.pc
$(pkgconfig_DATA): $(top_builddir)/config.status
CLEANFILES += rapicorn.pc
EXTRA_DIST += rapicorn.pc

# disable bogus cleanup checks
distcleancheck_listfiles = true # find . -type f -print
# distuninstallcheck_listfiles = true # find . -type f -print
distcheck-hook:
	$(MAKE) $(AM_MAKEFLAGS) -C birnet/ api-check
	$(MAKE) $(AM_MAKEFLAGS) -C rapicorn/ api-check
