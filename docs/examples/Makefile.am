# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE

# variables
noinst_PROGRAMS  = # auto-cleaned
noinst_DATA      =
XML_FILES        =
EXTRA_DIST      += $(XML_FILES) $(TEST_PYPROGS) $(EXAMPLES)
RPYTHON          = PYTHONPATH="$$PYTHONPATH:$(pythondir)" $(PYTHON)
X11TEST_RPYTHON  = $(X11TEST_SERVER) && rrun() { $(RPYTHON) "$$@" $(XLIBSTDERREXTFIX) ; } && rrun

EXAMPLES = sample.xml

# === tests ===
TEST_PYPROGS =
TEST_XMLUIS  = sample.xml
XML_FILES   += $(TEST_XMLUIS)

# === TEST_XMLUIS ===
.PHONY: check-TEST_XMLUIS
check-TEST_XMLUIS: $(top_builddir)/tools/rapidrun
	$(Q) $(X11TEST_SERVER) && \
	  for tst in $(TEST_XMLUIS) ; do \
	    [ -f ./"$$tst" ] && rt=./"$$tst" || rt="$(srcdir)/$$tst" ;   		\
	    $(top_builddir)/tools/rapidrun -x $$rt && echo "PASS: $$tst" || exit 1 ;	\
	  done
check-am: check-TEST_XMLUIS

# === TEST_PYPROGS ===
.PHONY: check-TEST_PYPROGS
check-TEST_PYPROGS:
	$(Q) $(X11TEST_SERVER) && \
	  for tst in $(TEST_PYPROGS) ; do \
	    [ -f ./"$$tst" ] && rt=./"$$tst" || rt="$(srcdir)/$$tst" ;   \
	    $(RPYTHON) $$rt && echo "PASS: $$tst" || exit 1 ; 		 \
	  done
	@MESSAGETEXT="All $(words $(TEST_PYPROGS)) Python tests passed"	 \
	&& [ 0 -lt $(words $(TEST_PYPROGS)) ]                            \
	&& echo $$MESSAGETEXT | sed 's/./=/g' && echo $$MESSAGETEXT      \
	&& echo $$MESSAGETEXT | sed 's/./=/g' || true
installcheck-local: check-TEST_PYPROGS

# === XML Linting ===
xmllint-check:
	cd $(srcdir) && [ -z "$(XML_FILES)" ] || $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES  += stamp-autochecks
