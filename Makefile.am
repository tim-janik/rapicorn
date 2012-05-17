# Rapicorn
# Copyright (C) 2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = . po plic rcore ui rope tools tests pytests examples docs web

docdir  = ${datarootdir}/doc/rapicorn@RAPICORN_RELEASE@

# === setup ===
AUTOMAKE_OPTIONS = 1.9 dist-bzip2 no-dist-gzip
configure: @DVLBUILD@ acrapicorn.m4	# acrapicorn.m4 is included by configure.in

# === web site ===
site site-preview site-update:
	@$(MAKE) -C web/ $(AM_MAKEFLAGS) $@
.PHONY: site site-update

test:
	$(MAKE) -C tests/ $(AM_MAKEFLAGS) $@
.PHONY: test

# === doc/ files ===
maindocsdir   = $(docdir)/
maindocs_DATA = README NEWS
EXTRA_DIST   += $(maindocs_DATA)

# === i18n tool files ===
CLEANFILES += intltool-extract intltool-merge intltool-update
EXTRA_DIST += intltool-extract.in intltool-merge.in intltool-update.in

# === rapicorn toplevel include file ===
toprapicorn_includedir      = $(includedir)/rapicorn@RAPICORN_RELEASE@/
toprapicorn_include_HEADERS = rapicorn-core.hh rapicorn.hh
EXTRA_DIST += $(toprapicorn_include_HEADERS)

# === rapicorn*.pc ===
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rapicorn@RAPICORN_RELEASE@.pc
rapicorn@RAPICORN_RELEASE@.pc: $(top_builddir)/config.status rapicorn.pc.in
	cd $(top_builddir) && $(SHELL) ./config.status --file=$@:rapicorn.pc.in
CLEANFILES += rapicorn@RAPICORN_RELEASE@.pc
EXTRA_DIST += rapicorn.pc.in

# === API version check ===
MAKE_API_CHECKSUM=$(strip							\
	( $(MAKE) -C $(top_srcdir)/rcore/ $(AM_MAKEFLAGS) api-content &&		\
	  $(MAKE) -C $(top_srcdir)/ui/   $(AM_MAKEFLAGS) api-content    ) | 	\
	md5sum | sed 's/ .*//'							\
)
api-check: # ensure headers don't change if RELEASE=outdated && REVISION=0
	cd $(srcdir) \
	&& RAPICORN_CURRENT_CHECKSUM=$$($(MAKE_API_CHECKSUM))			\
	&& RAPICORN_MONTHLY_VERSION="$$(expr $$(date +%Y%m) - 200000)"		\
	&& test "$$RAPICORN_CURRENT_CHECKSUM"  = "`cat api-check.md5`"		\
	|| if [ "$$RAPICORN_MONTHLY_VERSION" =					\
                "$$(expr @RAPICORN_RELEASE_YEAR@ '*' 100 + @RAPICORN_RELEASE_MONTH@)" \
		-o "@RAPICORN_REVISION_VERSION@" -gt 0 ]; then			\
	     echo "$$RAPICORN_CURRENT_CHECKSUM" > api-check.md5 ;		\
	   else \
	     MSG1="Rapicorn version needs updating (checksum: $$RAPICORN_CURRENT_CHECKSUM): " ; \
	     MSG2="@RAPICORN_RELEASE_YEAR@.@RAPICORN_RELEASE_MONTH@.@RAPICORN_REVISION_VERSION@ => " ; \
	     MSG3="$$[RAPICORN_MONTHLY_VERSION/100].$$[RAPICORN_MONTHLY_VERSION%100].0" ; \
	     MSG="$$MSG1$$MSG2$$MSG3" ; echo $$MSG | sed 's/./=/g'; echo $$MSG; echo $$MSG | sed 's/./=/g' ; \
	     exit 1 ; \
	   fi
api-checksum:
	$(MAKE_API_CHECKSUM)
.PHONY: api-check api-checksum
#check-am: api-check # check API version upon make check
distcheck-hook: # check API version upon make distcheck
	$(MAKE) $(AM_MAKEFLAGS) -C $(top_srcdir)/ api-check
installcheck-local:
	$(Q) cd . \
	&& echo '#include <rapicorn-core.hh>'				 > tmpx.cc \
	&& echo '#include <rapicorn.hh>'				>> tmpx.cc \
	&& echo 'using namespace Rapicorn;'				>> tmpx.cc \
	&& echo 'extern "C" int main (int argc, char *argv[]) {'	>> tmpx.cc \
	&& echo 'ApplicationH app = init_app (".AMtest", &argc, argv);'	>> tmpx.cc \
	&& echo 'return app.run_and_exit(); }'				>> tmpx.cc \
		; eval "$$TSTDIAGNOSE" "'Create  rapicorn sample program'"
	$(Q) $(CXX) tmpx.cc -c `PKG_CONFIG_PATH="$(pkgconfigdir):$$PKG_CONFIG_PATH" pkg-config --cflags rapicorn@RAPICORN_RELEASE@` \
		; eval "$$TSTDIAGNOSE" "'Compile rapicorn sample program'"
	$(Q) $(CXX) tmpx.o -o tmpx `PKG_CONFIG_PATH="$(pkgconfigdir):$$PKG_CONFIG_PATH" pkg-config --libs rapicorn@RAPICORN_RELEASE@` \
		; eval "$$TSTDIAGNOSE" "'Link    rapicorn sample program'"
	$(Q) LD_LIBRARY_PATH="$(libdir):$$LD_LIBRARY_PATH" ./tmpx \
		; eval "$$TSTDIAGNOSE" "'Execute rapicorn sample program'"
	$(Q) rm -f tmpx.o tmpx.cc tmpx

# === source code check ===
check-dynamic_cast:	# check for dynamic_cast of reference types which is broken in gcc <= 3.3.5
	$(Q) echo "  CHECK  Repel dynamic_cast of reference types..." \
	&& find $(srcdir)	-type f \( -iname '*.[hc][hc]' -o -iname '*.[hc]' \)	\
			-exec grep -Hn 'dynamic_cast[^<]*<[^>&]*&' {} \;	\
	| grep '.' ; exit $$[!$$?] # fail on matches
check-list_properties: # only public interfaces should export properties
	$(Q) cd $(srcdir) \
	&& echo "  CHECK  Repel untyped list_properties() method implementations..." \
	&& ! grep '^[[:space:]]*list_properties[[:space:]]*([^)]*)[^;]*$$' -n ui/*.cc rcore/*.cc
	$(Q) cd $(srcdir) \
	&& echo "  CHECK  Repel non-interface list_properties() method implementations..." \
	&& ! grep 'Impl::list_properties[[:space:]]*([^)]*)[^;]*$$' -n ui/*.cc rcore/*.cc \
	     --exclude ui/item.cc --exclude ui/container.cc --exclude ui/buttons.cc
check-am: check-dynamic_cast check-list_properties # run source code checks before all others

# === automake fixes ===
# disable bogus cleanup checks
distcleancheck_listfiles = true # find . -type f -print
# distuninstallcheck_listfiles = true # find . -type f -print

# === Release Rule Configuration ===
RELEASE_VERSION   = $(RAPICORN_VERSION)
RELEASE_REVNAME   = RAPICORN_REVISION_VERSION
RELEASE_DESTHOST  = testbit.eu
RELEASE_DESTPATH  = dist/rapicorn
RELEASE_CHANGELOG = ChangeLog
RELEASE_CHL_RANGE = 3c74b440cecaba3570eecc6728bbe319ba790b59..HEAD
# Makefile.decl introduced rules: ChangeLog release-upload
