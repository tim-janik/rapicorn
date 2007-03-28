# Rapicorn Examples
# Copyright (C) 2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES  	+= -I$(top_srcdir)
DEFS            += -DRAPICORN_PRIVATE -DPARANOID $(RAPICORN_CFLAGS) $(CONDITIONAL_CFLAGS)

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
programs_ldadd    = $(top_builddir)/rapicorn/librapicorn@LT_RELEASE@.la $(RAPICORN_LIBS)
gtkprograms_ldadd = $(programs_ldadd) $(GTK_LIBS)
GTK_EXAMPLES      =

# program sources:
GTK_EXAMPLES    += tour
tour_SOURCES     = tour.cc
tour_LDADD       = $(gtkprograms_ldadd)
XML_FILES       += tour.xml
EXTRA_DIST      += testpixs.c

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

noinst_DATA =

stamp-autochecks: $(XML_FILES)
	@$(MAKE) xml-lint
	@touch $@
noinst_DATA += stamp-autochecks

EXTRA_DIST += $(XML_FILES) testpic.jpg testpic.png testpic-IHDR.png testpic-IDAT.png testpic-DATA.png

# copy XML_FILES into builddir
stamp-xmlcopy: $(XML_FILES)
	test '$(srcdir)' = '.' || \
	  for file in $(XML_FILES); do \
	    cp $(srcdir)/$$file . ; \
	  done
	touch $@
noinst_DATA += stamp-xmlcopy
