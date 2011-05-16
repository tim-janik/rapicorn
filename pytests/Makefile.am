# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= -DRAPICORN_LOG_DOMAIN='"$(basename $(@F))"'

# variables
noinst_PROGRAMS  = # auto-cleaned
noinst_DATA      =
EXTRA_DIST       = $(TEST_PYPROGS)
XML_FILES        =
EXTRA_DIST      += $(XML_FILES)
RPYTHON          = PYTHONPATH="$$PYTHONPATH:$(pythondir)" $(PYTHON) # -c "import @RAPICORN_NAMESPACE_NAME@"
X11TEST_RPYTHON  = $(X11TEST_SERVER) && rrun() { $(RPYTHON) "$$@" $(XLIBSTDERREXTFIX) ; } && rrun


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
	@$(X11TEST_SERVER) && \
	  for tst in $(TEST_PYPROGS) ; do \
	    [ -f ./"$$tst" ] && rt=./"$$tst" || rt="$(srcdir)/$$tst" ;   \
	    $(RPYTHON) $$rt && echo "PASS: $$tst" || exit 1 ; 		 \
	  done
	@MESSAGETEXT="All $(words $(TEST_PYPROGS)) tests passed"	 \
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
