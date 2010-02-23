# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= -DRAPICORN_LOG_DOMAIN='"$(basename $(@F))"'

noinst_PROGRAMS  = $(EXAMPLES)
noinst_DATA      =
XML_FILES        =
EXAMPLES         =
EXTRA_DIST      += $(XML_FILES)

LDADDS   = $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la $(RAPICORN_DEP_LIBS)
RAPIDRUN = $(top_builddir)/tools/rapidrun
IMGCHECK = $(top_builddir)/core/tests/imgcheck
XVFB_RAPIDRUN = $(XVFB_START) && rrun() { $(RAPIDRUN) "$$@" $(XLIBSTDERREXTFIX) ; } && rrun

# === programs ===
EXAMPLES        += fileview
fileview_SOURCES = fileview.cc
fileview_LDADD   = $(LDADDS)
XML_FILES       += fileview.xml

# === test rules ===

# alignment test
# alignment.xml contains: snapshot_file="xtmp-alignment.png
XML_FILES  += alignment.xml
EXTRA_DIST += alignment.png
alignment-test: $(srcdir)/alignment.xml
	@$(call TSTTITLE, alignment.xml PNG test)
	$(_@) rm -f xtmp-alignment.png
	$(_@) $(XVFB_RAPIDRUN) -x $(srcdir)/alignment.xml $(STDERR_@)
	$(_@) $(IMGCHECK) $(srcdir)/alignment.png xtmp-alignment.png ; $(call TSTRESULT)
	$(_@) rm -f xtmp-alignment.png
check: alignment-test

# arrowsize test
arrowsize-test:
	@$(call TSTTITLE, arrowsize-test.xml)
	$(_@) $(XVFB_RAPIDRUN) -x $(srcdir)/arrowsize-test.xml ; $(call TSTRESULT)
EXTRA_DIST += arrowsize-test.xml
check: arrowsize-test

# factory-test
factory-test:
	@$(call TSTTITLE, FactoryTest)
	$(_@) $(XVFB_RAPIDRUN) -x --test-dump \
	  --test-matched-node=".*" $(srcdir)/factory-test.xml > $(TSTOUT)
	@$(call TSTCMP, $(srcdir)/factory-test.ref)
EXTRA_DIST += factory-test.xml factory-test.ref
check: factory-test

xmllint-check:
	@$(call TSTTITLE, linting $(XML_FILES))
	$(_@) cd $(srcdir) && $(XMLLINT) --noout $(XML_FILES) ; $(call TSTRESULT)
check: xmllint-check

.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES += stamp-autochecks
