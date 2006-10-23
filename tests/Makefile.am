# Rapicorn Tests
# Copyright (C) 2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir)
DEFS     += -DPARANOID $(RAPICORN_CFLAGS)

# test programs:
programs_ldadd = $(top_builddir)/rapicorn/librapicorn.la $(RAPICORN_LIBS) $(PANGO_LIBS) $(GTK_LIBS)
# program sources:
TESTS             += walker
walker_SOURCES     = walker.cc
walker_LDADD       = $(programs_ldadd)
TESTS             += markup
markup_SOURCES     = markup.cc
markup_LDADD       = $(programs_ldadd)
TESTS             += properties
properties_SOURCES = properties.cc
properties_LDADD   = $(programs_ldadd)
TESTS             += primitives
SLOWTESTS         += primitives
primitives_SOURCES = primitives.cc
primitives_LDADD   = $(programs_ldadd)
TESTS             += region
region_SOURCES     = region.cc
region_LDADD       = $(programs_ldadd)
TESTS             += shaderect
shaderect_SOURCES  = shaderect.cc
shaderect_LDADD    = $(programs_ldadd)
TESTS             += pixcombine
pixcombine_SOURCES = pixcombine.cc
pixcombine_LDADD   = $(programs_ldadd)
TESTS             += items
items_SOURCES      = items.cc
items_LDADD        = $(programs_ldadd)
TESTS             += testitems
testitems_SOURCES  = testitems.cc
testitems_LDADD    = $(programs_ldadd)
TESTS             += evaltest
evaltest_SOURCES   = evaltest.cc
evaltest_LDADD     = $(programs_ldadd)

# build tests
noinst_PROGRAMS = $(TESTS)

# run tests
test: check
