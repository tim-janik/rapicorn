# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= -DRAPICORN_LOG_DOMAIN='"$(basename $(@F))"'

noinst_PROGRAMS  = $(EXAMPLES)
noinst_DATA      =
XML_FILES        =
EXAMPLES         =
EXTRA_DIST      += $(XML_FILES)

LDADDS   = $(top_builddir)/rapicorn/librapicorn@RAPICORN_RELEASE@.la $(RAPICORN_DEP_LIBS)
RAPIDRUN = $(top_builddir)/tools/rapidrun
IMGCHECK = $(top_builddir)/rapicorn-core/tests/imgcheck

XVFB_RAPIDRUN = $(XVFB_START) && rrun() { echo "$(RAPIDRUN) $$@" && $(RAPIDRUN) "$$@" ; } && rrun

# === programs ===
EXAMPLES        += fileview
fileview_SOURCES = fileview.cc
fileview_LDADD   = $(LDADDS)
XML_FILES       += fileview.xml

# === test rules ===
# alignment test
alignment-test: $(srcdir)/alignment.xml
	rm -f xtmp-alignment.png
	@$(XVFB_RAPIDRUN) -x $(srcdir)/alignment.xml
	$(IMGCHECK) alignment.png xtmp-alignment.png
	rm -f xtmp-alignment.png
XML_FILES  += alignment.xml
EXTRA_DIST += alignment.png
check: alignment-test
# arrowsize test
arrowsize-test:
	@$(XVFB_RAPIDRUN) -x $(srcdir)/arrowsize-test.xml
check: arrowsize-test
# factory tests
factory-test:
	@$(XVFB_RAPIDRUN) -x $(srcdir)/factory-test.xml
check: factory-test

xmllint-check:
	cd $(srcdir) && $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES += stamp-autochecks
