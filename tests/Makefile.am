# Rapicorn Tests
# Copyright (C) 2005 Tim Janik

INCLUDES = -I$(top_srcdir)
DEFS     = -DPARANOID $(RAPICORN_CFLAGS) -DRAPICORN_PRIVATE

# test programs:
TESTS = properties walker markup primitives shaderect pixcombine
programs_ldadd = $(top_builddir)/rapicorn/librapicorn.la $(RAPICORN_LIBS) $(PANGO_LIBS) $(GTK_LIBS)
# program sources:
walker_SOURCES = walker.cc
walker_LDADD = $(programs_ldadd)
markup_SOURCES = markup.cc
markup_LDADD = $(programs_ldadd)
properties_SOURCES = properties.cc
properties_LDADD = $(programs_ldadd)
primitives_SOURCES = primitives.cc
primitives_LDADD = $(programs_ldadd)
shaderect_SOURCES = shaderect.cc
shaderect_LDADD = $(programs_ldadd)
pixcombine_SOURCES = pixcombine.cc
pixcombine_LDADD = $(programs_ldadd)

# build tests
noinst_PROGRAMS = $(TESTS)

# run tests
test: check
