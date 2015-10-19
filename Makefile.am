# Rapicorn
include $(top_srcdir)/Makefile.decl

SUBDIRS     = . po data res rcore aidacc ui cython tools tests pytests examples docs

docdir      = ${datarootdir}/doc/rapicorn-@MAJOR@
noinst_DATA =

# === web site ===
site site-preview site-update:
	@$(MAKE) -C web/ $(AM_MAKEFLAGS) $@
.PHONY: site site-update

test:
	$(MAKE) -C tests/ $(AM_MAKEFLAGS) $@
.PHONY: test

# === doc/ files ===
maindocsdir   = $(docdir)/
maindocs_DATA = README NEWS COPYING.MPL
README: README.md
	$(AM_V_GEN)
	$(Q) rm -f $@      # work around read-only target files
	$(Q) $(PANDOC) -S -f markdown_github+pandoc_title_block-hard_line_breaks -t plain $< -o $@
	$(Q) chmod a-w $@  # prevent accidental target file edits
.SECONDARY: README.md # pacify make(1) when shipping README without README.md
EXTRA_DIST   += $(maindocs_DATA)

# === i18n tool files ===
CLEANFILES += intltool-extract intltool-merge intltool-update
EXTRA_DIST += intltool-extract.in intltool-merge.in intltool-update.in

# === rapicorn toplevel include file ===
toprapicorn_includedir      = $(includedir)/rapicorn-@MAJOR@/
toprapicorn_include_HEADERS = rapicorn-core.hh rapicorn-test.hh rapicorn.hh
EXTRA_DIST += $(toprapicorn_include_HEADERS)

# === rapicorn*.pc ===
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rapicorn-@MAJOR@.pc
rapicorn-@MAJOR@.pc: $(top_builddir)/config.status rapicorn.pc.in
	cd $(top_builddir) && $(SHELL) ./config.status --file=$@:rapicorn.pc.in
CLEANFILES += rapicorn-@MAJOR@.pc
EXTRA_DIST += rapicorn.pc.in

# == ETAGS ==
ETAGS:
	rm -f ETAGS
	$(MAKE) local-ETAGS -C rcore/
	$(MAKE) local-ETAGS -C ui/
.PHONY: ETAGS

# === API version check ===
MAKE_API_CHECKSUM=$(strip							\
	( $(MAKE) -C $(top_srcdir)/rcore/ $(AM_MAKEFLAGS) api-content &&		\
	  $(MAKE) -C $(top_srcdir)/ui/   $(AM_MAKEFLAGS) api-content    ) | 	\
	md5sum | sed 's/ .*//'							\
)
api-checksum:
	$(MAKE_API_CHECKSUM)
.PHONY: api-checksum
distcheck-hook: # check API version upon make distcheck
	$(MAKE) $(AM_MAKEFLAGS) -C $(top_srcdir)/
installcheck-local:
	$(Q) cd . \
	&& echo '#include <rapicorn-core.hh>'				 > tmpx.cc \
	&& echo '#include <rapicorn-test.hh>'				>> tmpx.cc \
	&& echo '#include <rapicorn.hh>'				>> tmpx.cc \
	&& echo 'using namespace Rapicorn;'				>> tmpx.cc \
	&& echo 'extern "C" int main (int argc, char *argv[]) {'	>> tmpx.cc \
	&& echo 'ApplicationH app = init_app (".AMtest", &argc, argv);'	>> tmpx.cc \
	&& echo 'Blob blob = Blob::load (".");'				>> tmpx.cc \
	&& echo 'return app.run_and_exit(); }'				>> tmpx.cc \
		; eval "$$TSTDIAGNOSE" "'Create  rapicorn sample program'"
	$(Q) $(CXX) -Werror tmpx.cc -c `PKG_CONFIG_PATH="$(pkgconfigdir):$$PKG_CONFIG_PATH" pkg-config --cflags rapicorn-@MAJOR@` \
		; eval "$$TSTDIAGNOSE" "'Compile rapicorn sample program'"
	$(Q) $(CXX) -Werror tmpx.o -o tmpx `PKG_CONFIG_PATH="$(pkgconfigdir):$$PKG_CONFIG_PATH" pkg-config --libs rapicorn-@MAJOR@` \
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
check-am: check-dynamic_cast # run source code checks before all others

# === automake fixes ===
# disable bogus cleanup checks
distcleancheck_listfiles = true # find . -type f -print
# distuninstallcheck_listfiles = true # find . -type f -print
MAINTAINERCLEANFILES += intltool-extract.in  intltool-merge.in  intltool-update.in

# == ChangeLog & Release Rules ==
CHANGELOG_RANGE = $(shell git cat-file -e bb02c8c92f88a091ca8ca2002a5800c2102e5e1d && \
                    echo bb02c8c92f88a091ca8ca2002a5800c2102e5e1d..HEAD || echo HEAD)
ChangeLog: $(GITSTAMPS)
	$(AM_V_GEN)
	$(Q) git log --pretty='%ad  %an 	# %h%n%n%B%n' --abbrev=11 \
		--date=short --first-parent $(CHANGELOG_RANGE)	 > xgen-$(@F) # Generate ChangeLog with -prefixed records
	$(Q) sed 's/^/	/; s/^	// ; s/[[:space:]]\+$$// '	-i xgen-$(@F) # Tab-indent commit bodies, kill trailing whitespaces
	$(Q) sed '/^\s*$$/{ N; /^\s*\n\s*$$/D }'		-i xgen-$(@F) # Compress multiple newlines
	$(Q) mv xgen-$(@F) $@
	$(Q) test -s $@ || { mv $@ $@.empty ; ls -al --full-time $@.empty ; exit 1 ; }
noinst_DATA += ChangeLog
MAINTAINERCLEANFILES += ChangeLog
EXTRA_DIST  += ChangeLog
