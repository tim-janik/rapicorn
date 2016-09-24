# Rapicorn                      - non-recursive Makefile

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)
V_gen = @printf '  %-9s%s\n'

# == non-migrated subdirs ==
SUBDIRS		 = po . examples

# == make setup ==
DEFS			+= @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE -D__RAPICORN_BUILD__ -D__TOPDIR__="\"$(abs_top_srcdir)/\""
AM_CPPFLAGS		+= -I$(top_builddir) # -I$(top_srcdir)
AM_CXXFLAGS		+= $(RAPICORN_CORE_CFLAGS) $(SVG_DEPENDENCIES_CFLAGS)
lib_LTLIBRARIES 	 =
bin_PROGRAMS		 =
noinst_DATA		 =
noinst_SCRIPTS		 =
noinst_PROGRAMS 	 =
make_check_pytests	 =
make_check_xml_files	 =

# == project tools and paths ==
# objdir:		# Loading newly built Python modules requires accesss to libtool objdir
objdir			= .libs
XMANIFOLD		= $(top_srcdir)/aidacc/xmanifold.py
YAPPS			= $(PYTHON) $(top_srcdir)/yapps2_deb/yapps2.py
AIDACC_INTERN		= $(top_builddir)/aidacc/aidacc-intern
AIDACC_CXXSTUB_PY	= aidacc/CxxStub.py
WITH_PYTHONPATH 	= PYTHONPATH="$(abs_top_builddir)/cython/$(objdir):$$PYTHONPATH"
PYTHON_INTERN		= $(WITH_PYTHONPATH) $(PYTHON)
XVFB_PYTHON_INTERN	= $(WITH_PYTHONPATH) $(X11_ENV) $(PYTHON)
XVFB_RAPIDRUN		= $(X11_ENV) $(abs_top_builddir)/tools/rapidrun --fatal-warnings "$$@"

# == topbuildid.cc ==
topbuildid ::= $(shell $(top_srcdir)/misc/mkbuildid.sh)
topbuildid.cc: $(top_srcdir)/misc/mkbuildid.sh
	$(AM_V_GEN)
	$(Q) $(top_srcdir)/misc/mkbuildid.sh $@ $(DEPDIR)/topbuildid.cc.d
CLEANFILES += topbuildid.cc
-include $(DEPDIR)/topbuildid.cc.d
buildid:
	@echo $(topbuildid)

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
