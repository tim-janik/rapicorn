# Rapicorn Examples
# Copyright (C) 2005 Tim Janik

INCLUDES  	= -I$(top_srcdir)
DEFS            = -DRAPICORN_PRIVATE -DPARANOID $(RAPICORN_CFLAGS) $(CONDITIONAL_CFLAGS)

# test programs:
if WITH_GTK
GTK_EXAMPLES = tour
CONDITIONAL_CFLAGS = $(GTK_CFLAGS)
else
GTK_EXAMPLES = 
CONDITIONAL_CFLAGS =
endif
programs_ldadd     = $(top_builddir)/rapicorn/librapicorn.la $(RAPICORN_LIBS)
programs_gtk_ldadd = $(programs_ldadd) $(GTK_LIBS)

# build tests
noinst_PROGRAMS = $(GTK_EXAMPLES)

# program sources:
tour_SOURCES = tour.cc gtkrootwidget.cc
tour_LDADD = $(programs_gtk_ldadd)
EXTRA_DIST += gtkrootwidget.hh tour.xml

XML_FILES = $(strip	\
	tour.xml	\
)

XMLLINTCALL     = $(if $(XMLLINT),$(XMLLINT),true) # handle undefined XMLLINT
xml-lint:
	$(XMLLINTCALL) --noout $(XML_FILES)
.PHONY: xml-lint

stamp-allchecks: $(XML_FILES)
	@$(MAKE) xml-lint
	@touch $@

noinst_DATA = stamp-allchecks

EXTRA_DIST += testpic.jpg testpic.png testpic-IHDR.png testpic-IDAT.png testpic-DATA.png
