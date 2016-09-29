# Rapicorn                      - non-recursive Makefile

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)
V_gen = @printf '  %-9s%s\n'

# == non-migrated subdirs ==
SUBDIRS		 = po .

# == make setup ==
DEFS			+= @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE -D__RAPICORN_BUILD__ -D__TOPDIR__="\"$(topdir)/\""
AM_CPPFLAGS		+= -I$(top_builddir) # -I$(top_srcdir)
AM_CXXFLAGS		+= $(RAPICORN_CORE_CFLAGS) $(SVG_DEPENDENCIES_CFLAGS)
lib_LTLIBRARIES 	 =
bin_PROGRAMS		 =
bin_SCRIPTS		 =
noinst_DATA		 =
noinst_SCRIPTS		 =
noinst_PROGRAMS 	 =
make_check_pytests	 =
make_check_xml_files	 =
make_check_targets	 =

# == project tools and paths ==
# objdir:		# Loading newly built Python modules requires accesss to libtool objdir
objdir			= .libs
TAPTOOL 		= $(topdir)/taptool.sh
XMANIFOLD		= $(topdir)/aidacc/xmanifold.py
YAPPS			= $(PYTHON) $(topdir)/yapps2_deb/yapps2.py
AIDACC_INTERN		= $(top_builddir)/aidacc/aidacc-intern
AIDACC_CXXSTUB_PY	= aidacc/CxxStub.py
WITH_PYTHONPATH 	= PYTHONPATH="$(abs_top_builddir)/cython/$(objdir):$$PYTHONPATH"
PYTHON_INTERN		= $(WITH_PYTHONPATH) $(PYTHON)
XVFB_PYTHON_INTERN	= $(WITH_PYTHONPATH) $(X11_ENV) $(PYTHON)
XVFB_RAPIDRUN		= $(X11_ENV) $(abs_top_builddir)/tools/rapidrun --fatal-warnings "$$@"
IMGCHECK		= tools/imgcheck

# == topbuildid.cc ==
topbuildid ::= $(shell $(topdir)/misc/mkbuildid.sh)
topbuildid.cc: $(top_srcdir)/misc/mkbuildid.sh
	$(AM_V_GEN)
	$(Q) $(topdir)/misc/mkbuildid.sh $@ $(DEPDIR)/topbuildid.cc.d
CLEANFILES += topbuildid.cc
-include $(DEPDIR)/topbuildid.cc.d
buildid:
	@echo $(topbuildid)

# == make_check_targets ==
.PHONY: $(make_check_targets)
check-local: $(make_check_targets)

# == stamp-autocheck-xmllint ==
EXTRA_DIST	+= $(make_check_xml_files)
noinst_DATA	+= stamp-autocheck-xmllint
CLEANFILES	+= stamp-autocheck-xmllint
stamp-autocheck-xmllint: $(make_check_xml_files)
	$(Q) for XMLFILE in $? ; do \
	  $(XMLLINT) --noout "$$XMLFILE" ; eval "$$TSTDIAGNOSE" "$$XMLFILE" || exit $$? ; \
	done
	$(Q) touch $@

# == check-python-tests ==
check-python-tests:
EXTRA_DIST += $(make_check_pytests)
define make_check_pytests_TEMPLATE
$(1)--test: $(1)
	$$(Q) $$(XVFB_PYTHON_INTERN) $(1) ; eval "$$$$TSTDIAGNOSE" $(1)
.PHONY: $(1)--test
check-python-tests: $(1)--test
endef
$(foreach PYTEST, $(make_check_pytests), $(eval $(call make_check_pytests_TEMPLATE, $(PYTEST))))
.PHONY: check-python-tests
check-local: check-python-tests

# == ./ dependencies ==
rapicorn.hh: ui/clientapi.hh

# == include/ install ==
rapicorn_includedir      = $(includedir)/rapicorn-@MAJOR@/
rapicorn_include_HEADERS = rapicorn-core.hh rapicorn-test.hh rapicorn.hh
EXTRA_DIST		+= $(rapicorn_include_HEADERS)

# == rapicorn.pc ==
rapicorn_pkgconfigdir    = $(libdir)/pkgconfig
rapicorn_pkgconfig_DATA  = rapicorn-@MAJOR@.pc
EXTRA_DIST		+= rapicorn.pc.in
rapicorn-@MAJOR@.pc: rapicorn.pc.in $(top_builddir)/config.status
	$(AM_V_GEN)
	$(Q) cd $(top_builddir) && $(SHELL) ./config.status --file=$@:$<
CLEANFILES += rapicorn-@MAJOR@.pc
# install rapicorn.pc -> rapicorn-@MAJOR@.pc symlink
rapicorn-pc-install-hook:	# runs after install-data
	$(Q) cd $(DESTDIR)$(rapicorn_pkgconfigdir) && \
	  $(call SYMLINK_LATEST, rapicorn-[0-9][0-9].pc, rapicorn.pc) && \
	  test -r rapicorn.pc -a -L rapicorn.pc
install-data-hook: rapicorn-pc-install-hook
rapicorn-pc-uninstall-hook:	# runs after uninstall
	$(Q) cd $(DESTDIR)$(rapicorn_pkgconfigdir) && \
	  $(call SYMLINK_LATEST, rapicorn-[0-9][0-9].pc, rapicorn.pc) && \
	  test -r rapicorn.pc -o ! -e rapicorn.pc
uninstall-hook: rapicorn-pc-uninstall-hook

# == doc/ install ==
projectdocsdir   = $(docdir)/
projectdocs_DATA = README NEWS COPYING.MPL

# == README ==
README: README.md
	$(AM_V_GEN)
	$(Q) rm -f $@		# work around read-only target
	$(Q) $(PANDOC) -S -f markdown_github+pandoc_title_block-hard_line_breaks -t plain $< -o $@
	$(Q) chmod a-w $@	# prevent accidental target file edits
.SECONDARY: README.md		# pacify make(1) when shipping README without README.md

# == res/ ==
include res/Makefile.sub

# == aidacc/ ==
include aidacc/Makefile.sub

# == rcore/ ==
include rcore/Makefile.sub

# == ui/ ==
include ui/Makefile.sub

# == cython/ ==
include cython/Makefile.sub

# == tools/ ==
include tools/Makefile.sub

# == docs/ ==
include docs/Makefile.sub

# == tests/ ==
include tests/Makefile.sub

# == examples/ ==
include examples/Makefile.sub
