# Rapicorn
# Copyright (C) 2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = . rapicorn po examples tests docs web
noinst_DATA =

# setup automake
AUTOMAKE_OPTIONS = 1.9 dist-bzip2 no-dist-gzip

site site-preview site-update:
	@$(MAKE) -C web/ $(AM_MAKEFLAGS) $@
.PHONY: site site-update

test:
	$(MAKE) -C tests/ $(AM_MAKEFLAGS) $@
.PHONY: test

# extra dependancies
configure: acrapicorn.m4


CLEANFILES += intltool-extract intltool-merge intltool-update

EXTRA_DIST += intltool-extract.in intltool-merge.in intltool-update.in

# ChangeLog rule
LAST_COMMITID = 3c74b440cecaba3570eecc6728bbe319ba790b59
ChangeLog:	$(shell ls "$${GIT_DIR:-.git}/`git-symbolic-ref -q HEAD || echo HEAD`" 2>/dev/null )
	git-log --pretty='format:%ad %an 	# %H (%cn)%n%n%s%n%n%b' \
		${LAST_COMMITID}..HEAD		 > xgen-$(@F)
	sed 's/^/	/;s/^	//;/^[ 	]*<unknown>$$/d' -i xgen-$(@F)
	cp xgen-$(@F) $@ && rm -f xgen-$(@F)

noinst_DATA += ChangeLog
EXTRA_DIST  += ChangeLog

# install pkg-config files
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rapicorn@LT_RELEASE@.pc
rapicorn@LT_RELEASE@.pc: $(srcdir)/rapicorn.pc.in
rapicorn@LT_RELEASE@.pc: $(top_builddir)/config.status
	cd $(top_builddir) && $(SHELL) ./config.status --file=$@:rapicorn.pc.in
$(pkgconfig_DATA): $(top_builddir)/config.status
CLEANFILES += rapicorn@LT_RELEASE@.pc
EXTRA_DIST += rapicorn@LT_RELEASE@.pc


# disable bogus cleanup checks
distcleancheck_listfiles = true # find . -type f -print
# distuninstallcheck_listfiles = true # find . -type f -print
distcheck-hook:
	$(MAKE) $(AM_MAKEFLAGS) -C rapicorn/ api-check

check-drcast:	# check for dynamic_cast of reference types which is broken in gcc <= 3.3.5
	find $(srcdir)	-type f \( -iname '*.[hc][hc]' -o -iname '*.[hc]' \)	\
			-exec grep -Hn 'dynamic_cast[^<]*<[^>&]*&' {} \;	\
	| grep '.' ; exit $$[!$$?] # fail on matches
check-am: check-drcast # run check-drcast before other checks
