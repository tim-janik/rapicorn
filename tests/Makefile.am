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
RAPIDRUN_TEST_DUMP = $(strip \
  $(XVFB_START) && \
    rrun() { \
      $(RAPIDRUN) -x --test-dump "$$@" $(XLIBSTDERREXTFIX) ; \
    } && rrun \
)

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

TEST_DUMP_TARGETS =
NOUPDATE          = true

# Name, Src, Item, Tmp, Ref
TEST_DUMP_CMP_DIFF_SCRIPT = $(strip \
  TL=`printf %-69s "$(1):"` && TR=`printf %70s "$$TL"` && echo -n "$$TR" && \
    $(RAPIDRUN_TEST_DUMP) --test-matched-node="$(3)" $(2) > $(4) || exit $? && \
    ${NOUPDATE} || cp -v $(4) $(5) && \
    cmp --quiet $(5) $(4) && echo OK || \
    { echo FAIL; diff -up $(5) $(4); exit 1; } && \
    rm -f $(4); \
)

# factory-test
factory-test:
	@$(call TEST_DUMP_CMP_DIFF_SCRIPT, \
		FactoryTest, $(srcdir)/factory-test.xml, .* , \
		xtmp-factory.out, factory-test.ref)
TEST_DUMP_TARGETS += factory-test

# test-dump target handling
check: ${TEST_DUMP_TARGETS}
force-update:
	$(MAKE) NOUPDATE=false ${TEST_DUMP_TARGETS}

xmllint-check:
	cd $(srcdir) && $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES += stamp-autochecks
