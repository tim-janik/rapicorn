# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= $(RAPICORN_CFLAGS)

# example programs
noinst_PROGRAMS	 = $(EXAMPLES)
noinst_DATA	 =
EXAMPLES	 =
LDADDS		 = $(top_builddir)/rapicorn/librapicorn@LT_RELEASE@.la $(RAPICORN_DEP_LIBS)
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
EXAMPLES	+= tour
tour_SOURCES	 = tour.cc
tour_LDADD	 = $(LDADDS)
XML_FILES	+= tour.xml
EXTRA_DIST	+= testpixs.c testpic.jpg testpic.png testpic-IHDR.png testpic-IDAT.png testpic-DATA.png

xmllint-check:
	cd $(srcdir) && $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
