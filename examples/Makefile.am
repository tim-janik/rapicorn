# Rapicorn Examples
# Copyright (C) 2005 Tim Janik

srcdir          = .
INCLUDES  	= -I$(top_srcdir) # -I$(srcdir)/cairo
LDLIBS	       += $(LIBS)
DEFS            = -DRAPICORN_PRIVATE -DPARANOID

# program sources:
tour_SOURCES = tour.cc
tour_LDADD = $(programs_ldadd)
# test programs:
EXAMPLES = tour
programs_ldadd = $(top_builddir)/rapicorn/librapicorn.la

# build tests
noinst_PROGRAMS = $(EXAMPLES)
