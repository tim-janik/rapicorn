# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE

# variables
noinst_PROGRAMS  = # auto-cleaned
noinst_DATA      =
XML_FILES        =
EXTRA_DIST      += $(XML_FILES) $(TEST_PYPROGS) $(EXAMPLES)
FPYTHON          = PYTHONPATH="$(abs_top_builddir)/cython/${objdir}:$$PYTHONPATH" $(PYTHON)
# Loading newly built Python modules requires accesss to libtool objdir
objdir           = .libs

EXAMPLES = example-bind.py

# === tests ===
TEST_PYPROGS =

# === python-call-info ===
python-call-info:
	@echo "$(FPYTHON) # -c 'import Rapicorn'"


# === TEST_PYPROGS ===
.PHONY: check-TEST_PYPROGS
check-TEST_PYPROGS:
	$(Q) \
	  for tst in $(TEST_PYPROGS) ; do \
	    [ -f ./"$$tst" ] && rt=./"$$tst" || rt="$(srcdir)/$$tst" ;		\
	    $(X11_ENV) $(FPYTHON) $$rt && echo "PASS: $$tst" || exit 1 ;	\
	  done
	@MESSAGETEXT="All $(words $(TEST_PYPROGS)) Python tests passed"	 \
	&& [ 0 -lt $(words $(TEST_PYPROGS)) ]                            \
	&& echo $$MESSAGETEXT | sed 's/./=/g' && echo $$MESSAGETEXT      \
	&& echo $$MESSAGETEXT | sed 's/./=/g' || true
check-local: check-TEST_PYPROGS

# === XML Linting ===
xmllint-check:
	cd $(srcdir) && [ -z "$(XML_FILES)" ] || ! $(XMLLINT) --noout $(XML_FILES) 2>&1 | grep '.'
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES  += stamp-autochecks
