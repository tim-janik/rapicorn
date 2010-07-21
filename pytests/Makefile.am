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
#TEST_PROGS      += hello
#hello_SOURCES    = hello.py


# === XML Linting ===
xmllint-check:
	cd $(srcdir) && $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES  += stamp-autochecks

