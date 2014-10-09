# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE

# variables
noinst_PROGRAMS  = # auto-cleaned
noinst_DATA      =
XML_FILES        =
EXTRA_DIST      += $(XML_FILES) $(TEST_PYPROGS) $(EXAMPLES)
RPYTHON          = PYTHONPATH="$$PYTHONPATH:$(pythondir)" $(PYTHON) # -c "import @RAPICORN_NAMESPACE_NAME@"
X11TEST_RPYTHON  = $(X11TEST_SERVER) && rrun() { $(RPYTHON) "$$@" $(XLIBSTDERREXTFIX) ; } && rrun


EXAMPLES = example-bind.py

# === tests ===
TEST_PYPROGS = simple.py loop.py testrapicorn.py chekwidgets.py testgc.py testbind.py

# === python-call-info ===
python-call-info:
	@echo '$(RPYTHON) # -c "import @RAPICORN_NAMESPACE_NAME@"'


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
