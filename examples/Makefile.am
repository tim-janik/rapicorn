# Rapicorn Examples
# Copyright (C) 2005 Tim Janik

srcdir          = .
INCLUDES  	= -I$(top_srcdir) $(GTK_CFLAGS) # -I$(srcdir)/cairo
LDLIBS	       += $(GTK_LIBS) $(LIBS)
DEFS            = -DRAPICORN_PRIVATE -DPARANOID

# program sources:
tour_SOURCES = tour.cc gtkrootwidget.cc
tour_LDADD = $(programs_ldadd) $(GTK_LIBS)
# test programs:
if WITH_GTK
GTK_EXAMPLES = tour
else
GTK_EXAMPLES = 
endif
programs_ldadd = $(top_builddir)/rapicorn/librapicorn.la

# build tests
noinst_PROGRAMS = $(GTK_EXAMPLES)
