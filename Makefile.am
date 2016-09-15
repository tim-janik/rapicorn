# Rapicorn                      - non-recursive Makefile

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)

# == non-migrated subdirs ==
SUBDIRS		 = po . aidacc ui cython tools tests examples docs

# == make setup ==
DEFS	       += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE -D__RAPICORN_BUILD__
AM_CPPFLAGS    += -I$(top_builddir) # -I$(top_srcdir)
AM_CXXFLAGS    += $(RAPICORN_CORE_CFLAGS) $(SVG_DEPENDENCIES_CFLAGS)
lib_LTLIBRARIES	=
bin_PROGRAMS    =

# == tools ==
XMANIFOLD	= $(top_srcdir)/aidacc/xmanifold.py

# == topbuildid.cc ==
topbuildid ::= $(shell $(top_srcdir)/misc/mkbuildid.sh)
topbuildid.cc: $(top_srcdir)/misc/mkbuildid.sh
	$(AM_V_GEN)
	$(Q) $(top_srcdir)/misc/mkbuildid.sh $@ $(DEPDIR)/topbuildid.cc.d
CLEANFILES += topbuildid.cc
-include $(DEPDIR)/topbuildid.cc.d
buildid:
	@echo $(topbuildid)

# == res/ ==
include res/Makefile.sub

# == rcore/ ==
include rcore/Makefile.sub
