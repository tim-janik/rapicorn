# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_GUI_CFLAGS)

export VPATH # point builddir executables at srcdir to find their corresponding .xml files

# example programs
noinst_PROGRAMS	 = $(EXAMPLES)
noinst_DATA	 =
EXAMPLES	 =
LDADDS		 = $(top_builddir)/ui/librapicorn-@MAJOR@.la $(SVG_DEPENDENCIES_LIBS)
XML_FILES	 =
EXTRA_DIST	+= $(XML_FILES)

# programs
EXAMPLES	+= hello
hello_SOURCES	 = hello.cc
hello_LDADD	 = $(LDADDS)
XML_FILES	+= hello.xml
EXAMPLES	+= scroller
scroller_SOURCES = scroller.cc
scroller_LDADD	 = $(LDADDS)
XML_FILES	+= scroller.xml
EXAMPLES	+= texttest
texttest_SOURCES = texttest.cc
texttest_LDADD	 = $(LDADDS)
XML_FILES	+= texttest.xml
EXAMPLES	+= graphics
graphics_SOURCES = graphics.cc
graphics_LDADD	 = $(LDADDS)
XML_FILES	+= graphics.xml
EXAMPLES	+= tour
tour_SOURCES	 = tour.cc
tour_LDADD	 = $(LDADDS)
XML_FILES	+= tour.xml
EXTRA_DIST	+= testpic.jpg testpic.png testpic-IHDR.png testpic-IDAT.png testpic-DATA.png

# == fileview ==
EXAMPLES        += fileview
fileview_SOURCES = fileview.cc
EXTRA_DIST      += liststore.cc
fileview_LDADD   = $(LDADDS)
XML_FILES       += fileview.xml

# tests
tour-test: tour
	$(Q) $(X11_ENV) ./tour --test-automation > $(TSTOUT) \
	&& fgrep -q "Rapicorn Tour" $(TSTOUT) ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f $(TSTOUT)
check: tour-test

xmllint-check:
	cd $(srcdir) && ! $(XMLLINT) --noout $(XML_FILES) 2>&1 | grep '.'
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES += stamp-autochecks
