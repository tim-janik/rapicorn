# Rapicorn Tests
# Copyright (C) 2005 Tim Janik

INCLUDES  	= -I$(top_srcdir) # -I$(srcdir)/cairo
LDLIBS	       += $(LIBS)
DEFS            = -DRAPICORN_PRIVATE -DPARANOID

# program sources:
walker_SOURCES = walker.cc
walker_LDADD = $(programs_ldadd)
signal_SOURCES = signal.cc
signal_LDADD = $(programs_ldadd)
markup_SOURCES = markup.cc
markup_LDADD = $(programs_ldadd)
datalist_SOURCES = datalist.cc
datalist_LDADD = $(programs_ldadd)
properties_SOURCES = properties.cc
properties_LDADD = $(programs_ldadd)
primitives_SOURCES = primitives.cc
primitives_LDADD = $(programs_ldadd)
# test programs:
TESTS = properties signal walker markup datalist primitives
programs_ldadd = $(top_builddir)/rapicorn/librapicorn.la $(PANGO_LIBS)

# build tests
noinst_PROGRAMS = $(TESTS)

# run tests
test: check
