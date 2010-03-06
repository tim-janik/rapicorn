# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(PYTHON_CPPFLAGS)
DEFS	  = -DPYRAPICORN=pyRapicorn -DRAPICORN_INTERNALS -DRAPICORN_LOG_DOMAIN=\"RAPICORN\" # "


pyrapicorn_cc_sources = rpytypes.cc
EXTRA_DIST += rpy.hh


# === python module ===
pyrapicorndir          = ${pythondir}/@RAPICORN_NAMESPACE_NAME@


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
installcheck-local: call-info # test-pywindow
