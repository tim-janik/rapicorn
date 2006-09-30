# Rapicorn Examples
# Copyright (C) 2005 Tim Janik

INCLUDES  	= -I$(top_srcdir)
DEFS            = -DRAPICORN_PRIVATE -DPARANOID $(RAPICORN_CFLAGS) $(CONDITIONAL_CFLAGS)

# test programs:
if WITH_GTK
CONDITIONAL_PROGS  = $(GTK_EXAMPLES)
CONDITIONAL_CFLAGS = $(GTK_CFLAGS)
else
CONDITIONAL_PROGS  =
CONDITIONAL_CFLAGS =
endif
XML_FILES =
# build tests
noinst_PROGRAMS   = $(CONDITIONAL_PROGS)
programs_ldadd    = $(top_builddir)/rapicorn/librapicorn.la $(RAPICORN_LIBS)
gtkprograms_ldadd = $(programs_ldadd) $(GTK_LIBS)

# program sources:
GTK_EXAMPLES    += tour
tour_SOURCES     = tour.cc
tour_LDADD       = $(gtkprograms_ldadd)
XML_FILES       += tour.xml

GTK_EXAMPLES    += scroller
scroller_SOURCES = scroller.cc
scroller_LDADD   = $(gtkprograms_ldadd)
XML_FILES       += scroller.xml

GTK_EXAMPLES    += texttest
texttest_SOURCES = texttest.cc
texttest_LDADD   = $(gtkprograms_ldadd)
XML_FILES       += texttest.xml

XMLLINTCALL = $(if $(XMLLINT),$(XMLLINT),true) # handle undefined XMLLINT
xml-lint:
	$(XMLLINTCALL) --noout $(XML_FILES)
.PHONY: xml-lint

stamp-allchecks: $(XML_FILES)
	@$(MAKE) xml-lint
	@touch $@

noinst_DATA = stamp-allchecks

EXTRA_DIST += $(XML_FILES) testpic.jpg testpic.png testpic-IHDR.png testpic-IDAT.png testpic-DATA.png
