# Rapicorn
# Copyright (C) 2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = . po rapicorn-core rapicorn tools examples docs web
noinst_DATA =

# === setup ===
AUTOMAKE_OPTIONS = 1.9 dist-bzip2 no-dist-gzip
configure: acrapicorn.m4	# acrapicorn.m4 is included by configure.in

# === web site ===
site site-preview site-update:
	@$(MAKE) -C web/ $(AM_MAKEFLAGS) $@
.PHONY: site site-update

test:
	$(MAKE) -C tests/ $(AM_MAKEFLAGS) $@
.PHONY: test


CLEANFILES += intltool-extract intltool-merge intltool-update
EXTRA_DIST += intltool-extract.in intltool-merge.in intltool-update.in

# === ChangeLog ===
LAST_COMMITID = 3c74b440cecaba3570eecc6728bbe319ba790b59
ChangeLog:	$(shell ls "$${GIT_DIR:-.git}/`git-symbolic-ref -q HEAD || echo HEAD`" 2>/dev/null )
	git-log --pretty='format:%ad %an 	# %H (%cn)%n%n%s%n%n%b' \
		${LAST_COMMITID}..HEAD		 > xgen-$(@F)
	sed 's/^/	/;s/^	//;/^[ 	]*<unknown>$$/d' -i xgen-$(@F)
	cp xgen-$(@F) $@ && rm -f xgen-$(@F)
noinst_DATA += ChangeLog
EXTRA_DIST  += ChangeLog

# === rapicorn*.pc ===
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rapicorn@LT_RELEASE@.pc
rapicorn@LT_RELEASE@.pc: $(top_builddir)/config.status rapicorn.pc.in
	cd $(top_builddir) && $(SHELL) ./config.status --file=$@:rapicorn.pc.in
CLEANFILES += rapicorn@LT_RELEASE@.pc
EXTRA_DIST += rapicorn.pc.in

# === API version check ===
MAKE_API_CHECKSUM=$(strip								\
	( $(MAKE) -C $(top_srcdir)/rapicorn-core/ $(AM_MAKEFLAGS) api-content &&     	\
	  $(MAKE) -C $(top_srcdir)/rapicorn/      $(AM_MAKEFLAGS) api-content    ) | 	\
	md5sum | sed 's/ .*//'								\
)
api-check: # ensure headers don't change if RELEASE=outdated && REVISION=0
	cd $(srcdir) \
	&& RAPICORN_CURRENT_CHECKSUM=$$($(MAKE_API_CHECKSUM))			\
	&& RAPICORN_WEEK_VERSION="$$(expr $$(date +%G) - 2000)$$(date +%V)"	\
	&& test "$$RAPICORN_CURRENT_CHECKSUM"  = "`cat api-check.md5`"		\
	|| if [ "$$RAPICORN_WEEK_VERSION" =					\
                "$$[@RAPICORN_RELEASE_YEAR@ * 100 + @RAPICORN_RELEASE_WEEK@]"	\
		-o "@RAPICORN_REVISION_VERSION@" -gt 0 ]; then			\
	     echo "$$RAPICORN_CURRENT_CHECKSUM" > api-check.md5 ;		\
	   else \
	     MSG1="The Rapicorn version (@RAPICORN_RELEASE_YEAR@.@RAPICORN_RELEASE_WEEK@.@RAPICORN_REVISION_VERSION@) needs updating " ; \
	     MSG2="(expected $$[RAPICORN_WEEK_VERSION/100].$$[RAPICORN_WEEK_VERSION%100].0) to match header changes (checksum: $$RAPICORN_CURRENT_CHECKSUM)" ; \
	     MSG="$$MSG1$$MSG2" ; echo $$MSG | sed 's/./=/g'; echo $$MSG; echo $$MSG | sed 's/./=/g' ; \
	     exit 1 ; \
	   fi
api-checksum:
	$(MAKE_API_CHECKSUM)
.PHONY: api-check api-checksum
#check-am: api-check # check API version upon make check
distcheck-hook: # check API version upon make distcheck
	$(MAKE) $(AM_MAKEFLAGS) -C $(top_srcdir)/ api-check

# === dynamic_cast check ===
check-drcast:	# check for dynamic_cast of reference types which is broken in gcc <= 3.3.5
	find $(srcdir)	-type f \( -iname '*.[hc][hc]' -o -iname '*.[hc]' \)	\
			-exec grep -Hn 'dynamic_cast[^<]*<[^>&]*&' {} \;	\
	| grep '.' ; exit $$[!$$?] # fail on matches
check-am: check-drcast # run check-drcast before other checks

# === automake fixes ===
# disable bogus cleanup checks
distcleancheck_listfiles = true # find . -type f -print
# distuninstallcheck_listfiles = true # find . -type f -print
