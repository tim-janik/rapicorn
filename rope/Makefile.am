# Rapicorn Rope
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

INCLUDES    += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_ROPE_CFLAGS) $(RAPICORN_GUI_CFLAGS) $(PYTHON_CPPFLAGS)

AIDACC       = $(top_builddir)/aidacc/aidacc-intern
RAPICORNIDL  = $(srcdir)/../ui/interfaces.idl
HARDCODEDIDL = $(srcdir)/../ui/hardcoded.idl

# === Cxxrapicorn ===
PyRapicorndir          = ${pythondir}/@RAPICORN_NAMESPACE_NAME@
PyRapicorn_PYTHON      = __init__.py pyrapicorn.py
PyRapicorn_LTLIBRARIES = __pyrapicorn.la
$(PyRapicorn_LTLIBRARIES): $(PyRapicorn_PYTHON) # force python source creation
python-call-info:
	@echo 'PYTHONPATH="$$PYTHONPATH:$(pythondir)" python # -c "import @RAPICORN_NAMESPACE_NAME@"'

# == pyrapicorn.py & pyrapicorn.cc ==
pyrapicorn.py pyrapicorn.cc: $(RAPICORNIDL) $(srcdir)/../aidacc/PyStub.py
	@: # compile IDL definitions into RapicornYYMM.{cc,py}
	$(Q) ${AIDACC} -x $(srcdir)/../aidacc/PyStub $(RAPICORNIDL) -o pyrapicorn \
	  -G strip-path=$(abs_top_srcdir)/ -G apikey=$(RAPICORN_NAMESPACE_NAME)
CLEANFILES += pyrapicorn.py pyrapicorn.cc
$(srcdir)/py-rope.cc: pyrapicorn.cc

# == __pyrapicorn from pyrapicorn.cc ==
__pyrapicorn_la_SOURCES	= py-rope.cc # includes pyrapicorn.cc
__pyrapicorn_la_LIBADD	= $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la
__pyrapicorn_la_LDFLAGS	= -module -avoid-version -no-undefined $(SYMBOLIC_LDFLAGS) $(AM_LDFLAGS)
EXTRA_DIST             += py-rope.hh

# === TESTS ===
# test PyStub file generation for testpass.idl
EXTRA_DIST += testpystub.ref testcxxpystub.ref
check-PyStub-testpass-generation: $(srcdir)/testpystub.ref $(srcdir)/testcxxpystub.ref
	@: # generate .cc and .py
	$(Q) ${AIDACC} -x $(srcdir)/../aidacc/PyStub.py $(srcdir)/../aidacc/tests/testpass.idl -o testpass \
	  -G strip-path=$(abs_top_srcdir)/ \
	; eval "$$TSTDIAGNOSE_LOGTEST"
	@: # check .py generation
	$(Q) mv testpass.py $(TSTOUT)
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testpystub.ref'"
	@: # check .cc generation
	$(Q) mv testpass.cc $(TSTOUT) \
	&& echo "static AIDA_UNUSED PyMethodDef dummy[] = { AIDA_PYSTUB_METHOD_DEFS(), { 0 } };" >>$(TSTOUT) \
	; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testcxxpystub.ref'"
check-local: check-PyStub-testpass-generation
#testcxxpystub.ref testpystub.ref: check-PyStub-testpass-generation # force-check-update dependency
# test PyStub file syntax
noinst_LTLIBRARIES    = __testpass.la
__testpass_la_SOURCES = testpystub.cc	# forces C++ syntax check
__testpass_la_LIBADD  = $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la
__testpass_la_LDFLAGS = -module -avoid-version -no-undefined $(SYMBOLIC_LDFLAGS) -rpath /nowhere $(AM_LDFLAGS) # -rpath forces .so
testpystub.cc: testcxxpystub.ref
	$(Q) $(MAKE) $(AM_MAKEFLAGS) check-PyStub-testpass-generation # force testcxxpystub.ref check before using it
	$(Q) cp $(srcdir)/testcxxpystub.ref xgen-$@
	$(Q) echo "static PyMethodDef __testpass_methods__[] = { AIDA_PYSTUB_METHOD_DEFS(), { 0 } };" >> xgen-$@
	$(Q) echo 'PyMODINIT_FUNC init__testpass (void) { Py_InitModule ("__testpass", __testpass_methods__); }' >> xgen-$@
	$(Q) mv xgen-$@ $@
check-PyStub-testpass-module: __testpass.la
	@: # check .cc syntax: already done by building __testpass.so
	@: # check .py syntax
	$(Q) cp .libs/__testpass.so .
	$(Q) $(PYTHON) -c "import py_compile; py_compile.compile ('$(srcdir)/testpystub.ref', 'testpystub.pyc')" \
	&& $(PYTHON) $(srcdir)/testpystub.ref ; eval "$$TSTDIAGNOSE"
	@: # cleanups
	$(Q) rm -f testpystub.cc __testpass.so
CLEANFILES += testpystub.cc __testpass.so testpystub.pyc
check-local: check-PyStub-testpass-module
# test PyStub files for Rapicorn interfaces
check-rapicorn-idl: __pyrapicorn.la pyrapicorn.py
	$(Q) ${AIDACC} $(RAPICORNIDL) > /dev/null ; eval "$$TSTDIAGNOSE"
	$(Q) cp .libs/__pyrapicorn.so .
	$(Q) $(PYTHON) pyrapicorn.py ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f __pyrapicorn.so
CLEANFILES += __pyrapicorn.so
check-local: check-rapicorn-idl
