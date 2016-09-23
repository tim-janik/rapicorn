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
noinst_SCRIPTS		 =
noinst_PROGRAMS 	 =
make_check_pytests	 =

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

# == topbuildid.cc ==
topbuildid ::= $(shell $(top_srcdir)/misc/mkbuildid.sh)
topbuildid.cc: $(top_srcdir)/misc/mkbuildid.sh
	$(AM_V_GEN)
	$(Q) $(top_srcdir)/misc/mkbuildid.sh $@ $(DEPDIR)/topbuildid.cc.d
CLEANFILES += topbuildid.cc
-include $(DEPDIR)/topbuildid.cc.d
buildid:
	@echo $(topbuildid)

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
