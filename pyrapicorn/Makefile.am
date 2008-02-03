# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(RAPICORN_DEP_CFLAGS) $(PYTHON_INCLUDES)
DEFS		 = -DPYRAPICORN=py@RAPICORN_NAMESPACE_NAME@ -DRAPICORN_INTERNALS -DRAPICORN_LOG_DOMAIN=\"RAPICORN\" # "


pyrapicorn_cc_sources = module.cc


# === python module ===
pyrapicorndir                          = ${pythondir}/@RAPICORN_NAMESPACE_NAME@
pyrapicorn_PYTHON                      = __init__.py
pyrapicorn_LTLIBRARIES                 = py@RAPICORN_NAMESPACE_NAME@.la
py@RAPICORN_NAMESPACE_NAME@_la_SOURCES = $(pyrapicorn_cc_sources)
py@RAPICORN_NAMESPACE_NAME@_la_LDFLAGS = -module -avoid-version -no-undefined -export-symbols-regex py@RAPICORN_NAMESPACE_NAME@
py@RAPICORN_NAMESPACE_NAME@_la_LIBADD  = $(top_builddir)/rapicorn/librapicorn@LT_RELEASE@.la


# === tests ===
test-pyprintout:
	@echo -n "Test Rapicorn::printout(): "
	@NL=$$'\n'; PYTHONPATH="$$PYTHONPATH:$(pythondir)" python -c "if 1:$$NL\
	  import @RAPICORN_NAMESPACE_NAME@ as Rapicorn $$NL\
	  Rapicorn.pyRapicorn0752.printout ('test_01')" \
	| fgrep -q test_01
	@echo OK
call-info:
	@echo 'PYTHONPATH="$$PYTHONPATH:$(pythondir)" python -c "import @RAPICORN_NAMESPACE_NAME@"'
check-local: test-pyprintout call-info
