# Rapicorn Tests
# Copyright (C) 2005 Tim Janik

srcdir          = .
INCLUDES  	= -I$(top_srcdir) # -I$(srcdir)/cairo
LDLIBS	       += $(LIBS)

# test programs:
programs_ldadd = $(top_builddir)/rapicorn/librapicorn.la
TESTS = walker
# program sources:
walker_SOURCES = walker.cc
walker_LDADD = $(programs_ldadd)

# build tests
noinst_PROGRAMS = $(TESTS)

# run tests
test: check
