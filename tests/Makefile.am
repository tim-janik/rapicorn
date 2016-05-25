# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_GUI_CFLAGS)

noinst_PROGRAMS  = benchaida $(EXAMPLES)
noinst_DATA      =
XML_FILES        =
EXAMPLES         =
EXTRA_DIST      += $(XML_FILES)

LDADDS   = $(top_builddir)/ui/librapicorn-@MAJOR@.la $(RAPICORN_DEP_LIBS)
RAPIDRUN = $(top_builddir)/tools/rapidrun
IMGCHECK = $(top_builddir)/ui/tests/imgcheck
X11TEST_RAPIDRUN = $(X11_ENV) $(RAPIDRUN) --fatal-warnings "$$@"

# == benchaida ==
benchaida_SOURCES  = benchaida.cc
benchaida_LDADD    = $(LDADDS)

# === test rules ===

# ui XML parsing and evaluation tests
XML_FILES += uitests.xml
ui-xml-test:
	$(Q) $(X11TEST_RAPIDRUN) -x $(srcdir)/uitests.xml ; eval "$$TSTDIAGNOSE"
check: ui-xml-test

# == Alignment test ==
# alignment.xml contains: snapshot_file="xtmp-alignment.png
XML_FILES  += alignment.xml
EXTRA_DIST += alignment.png
alignment-png-test: $(srcdir)/alignment.xml $(srcdir)/alignment.png
	$(Q) rm -f xtmp-alignment.png
	$(Q) $(X11TEST_RAPIDRUN) -x $(srcdir)/alignment.xml
	$(Q) $(IMGCHECK) $(srcdir)/alignment.png xtmp-alignment.png ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f xtmp-alignment.png
check: alignment-png-test

# == t700-png-image ==
t700-png-image-test: $(srcdir)/t700-png-image.xml $(srcdir)/t700-png-image.png # $(IMGCHECK) $(X11TEST_RAPIDRUN)
	$(Q) rm -f t700-png-image-snap.png
	$(Q) test $(srcdir) = $(builddir) || cp $(srcdir)/t700-png-image.png $(builddir)
	$(Q) export RAPIDRUN_RES=./ ; $(X11TEST_RAPIDRUN) -x $(srcdir)/t700-png-image.xml
	$(Q) test $(srcdir) = $(builddir) || rm -f $(builddir)/t700-png-image.png
	$(Q) $(IMGCHECK) $(srcdir)/t700-png-image.png t700-png-image-snap.png ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f t700-png-image-snap.png
XML_FILES  += t700-png-image.xml
EXTRA_DIST += t700-png-image.png
check: t700-png-image-test

# arrowsize test
arrowsize-test:
	$(Q) $(X11TEST_RAPIDRUN) -x $(srcdir)/arrowsize-test.xml ; eval "$$TSTDIAGNOSE"
EXTRA_DIST += arrowsize-test.xml
check: arrowsize-test

# factory-test
factory-test: $(srcdir)/factory-test.ref
	$(Q) $(X11TEST_RAPIDRUN) -x --test-dump \
	  --test-matched-node=".*" $(srcdir)/factory-test.xml > $(TSTOUT) ; eval "$$TSTDIAGNOSE"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/factory-test.ref'" "'$(TSTOUT)'"
EXTRA_DIST += factory-test.xml factory-test.ref
check: factory-test

# texting test
XML_FILES  += texting.xml
texting-png-test: $(srcdir)/texting.xml
	$(Q) rm -f xtmp-texting.png
	$(Q) $(X11TEST_RAPIDRUN) --snapshot=xtmp-texting.png -x $(srcdir)/texting.xml $(QSTDERR)
	$(Q) eval "$$TSTOCR" xtmp-texting.png "Regu.ar.Text" \""Insensitive Text"\" \""Editab.e Text Edit.r Text"\"
	$(Q) rm -f xtmp-texting.png
check: texting-png-test

xmllint-check:
	$(Q) cd $(srcdir) && ! $(XMLLINT) --noout $(XML_FILES) 2>&1 | grep '.'
check: xmllint-check

.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES += stamp-autochecks
