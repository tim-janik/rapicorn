# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_CORE_CFLAGS) $(SVG_DEPENDENCIES_CFLAGS)

# test programs
noinst_PROGRAMS	 = $(TAPTESTS)
LDADDS		 = $(top_builddir)/rcore/librapicorncore-@MAJOR@.la $(SVG_DEPENDENCIES_LIBS)

