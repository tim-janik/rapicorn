# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= -DRAPICORN_LOG_DOMAIN='"$(basename $(@F))"'

# variables
noinst_PROGRAMS  = $(TEST_PROGS)
noinst_DATA      =
XML_FILES        =
EXTRA_DIST      += $(XML_FILES)
RPYTHON          = PYTHONPATH="$$PYTHONPATH:$(pythondir)" $(PYTHON) # -c "import @RAPICORN_NAMESPACE_NAME@"
XVFB_RPYTHON     = $(XVFB_START) && rrun() { $(RPYTHON) "$$@" $(XLIBSTDERREXTFIX) ; } && rrun


# === tests ===
TEST_PYPROGS =
TEST_PYPROGS     += simple.py
simple_py_SOURCES = simple.py
TEST_PYPROGS     += loop.py
loop_py_SOURCES   = loop.py


# === python-call-info ===
python-call-info:
	@echo '$(RPYTHON) # -c "import @RAPICORN_NAMESPACE_NAME@"'


# === TEST_PYPROGS ===
.PHONY: check-TEST_PYPROGS
check-TEST_PYPROGS:
	@$(XVFB_START) && \
	  for tst in $(TEST_PYPROGS) ; do \
	    $(RPYTHON) ./$$tst && echo "PASS: $$tst" || exit 1 ;   \
	  done
	@MESSAGETEXT="All $(words $(TEST_PROGS)) tests passed"  \
	&& [ 0 -lt $(words $(TEST_PROGS)) ]                             \
	&& echo $$MESSAGETEXT | sed 's/./=/g' && echo $$MESSAGETEXT     \
	&& echo $$MESSAGETEXT | sed 's/./=/g' || true
check: check-TEST_PYPROGS


# === XML Linting ===
xmllint-check:
	cd $(srcdir) && $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES  += stamp-autochecks

