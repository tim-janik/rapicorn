# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(RAPICORN_DEP_CFLAGS) $(PYTHON_INCLUDES)
DEFS	  = -DPYRAPICORN=pyRapicorn -DRAPICORN_INTERNALS -DRAPICORN_LOG_DOMAIN=\"RAPICORN\" # "


pyrapicorn_cc_sources = rpymodule.cc rpytypes.cc
EXTRA_DIST += rpy.hh


# === python module ===
pyrapicorndir          = ${pythondir}/@RAPICORN_NAMESPACE_NAME@
pyrapicorn_PYTHON      = __init__.py
pyrapicorn_LTLIBRARIES = pyRapicorn.la
pyRapicorn_la_SOURCES  = $(pyrapicorn_cc_sources)
pyRapicorn_la_LDFLAGS  = -module -avoid-version -no-undefined -export-symbols-regex pyRapicorn
pyRapicorn_la_LIBADD   = $(top_builddir)/rapicorn/librapicorn@RAPICORN_RELEASE@.la


# === tests ===
test-pywindow:
	@echo -n "Test Rapicorn.Application: "
	@NL=$$'\n'; PYTHONPATH="$$PYTHONPATH:$(pythondir)" ${PYTHON} -c "if 1:$$NL\
	  from @RAPICORN_NAMESPACE_NAME@ import Application $$NL\
	  w = Application.create_window ('Root') $$NL\
	  w.show() $$NL" \
	| fgrep Window-new >/dev/null
	@echo OK
call-info:
	@echo 'PYTHONPATH="$$PYTHONPATH:$(pythondir)" python -c "import @RAPICORN_NAMESPACE_NAME@"'
installcheck-local: test-pywindow call-info
