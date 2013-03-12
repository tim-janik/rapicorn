# Rapicorn Rope
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

INCLUDES   += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS       += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS = $(RAPICORN_ROPE_CFLAGS) $(RAPICORN_GUI_CFLAGS) $(PYTHON_CPPFLAGS)

AIDACC       = $(top_builddir)/aidacc/aidacc-intern
RAPICORNIDL  = $(srcdir)/../ui/interfaces.idl
HARDCODEDIDL = $(srcdir)/../ui/hardcoded.idl

# === Cxxrapicorn ===
cxxrapicorn_cc_sources = py-rope.cc
EXTRA_DIST           += py-rope.hh
cxxrapicorndir          = ${pythondir}/@RAPICORN_NAMESPACE_NAME@
cxxrapicorn_PYTHON      = __init__.py pyrapicorn.py loop.py
cxxrapicorn_LTLIBRARIES = cxxrapicorn.la
cxxrapicorn_la_SOURCES  = $(cxxrapicorn_cc_sources)
cxxrapicorn_la_LIBADD   = $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la
cxxrapicorn_la_LDFLAGS  = $(strip				\
	-module -avoid-version -export-symbols-regex cxxrapicorn \
	-no-undefined $(SYMBOLIC_LDFLAGS)			\
)
$(cxxrapicorn_LTLIBRARIES): $(cxxrapicorn_PYTHON) # force python source creation
python-call-info:
	@echo 'PYTHONPATH="$$PYTHONPATH:$(pythondir)" python # -c "import @RAPICORN_NAMESPACE_NAME@"'

# === pyrapicorn.cc ===
pyrapicorn.py pyrapicorn.cc: $(RAPICORNIDL) $(srcdir)/../aidacc/PyStub.py
	@: # compile IDL definitions into RapicornYYMM.{cc,py}
	$(Q) ${AIDACC} -x $(srcdir)/../aidacc/PyStub $(RAPICORNIDL) -o pyrapicorn
CLEANFILES += pyrapicorn.py pyrapicorn.cc
$(srcdir)/py-rope.cc: pyrapicorn.cc

# === TESTS ===
# test PyStub files for testpass.idl
EXTRA_DIST += testpystub.ref testcxxpystub.ref
check-PyStub-testpass: $(srcdir)/testpystub.ref $(srcdir)/testcxxpystub.ref
	@: # generate .cc and .py
	$(Q) ${AIDACC} -x $(srcdir)/../aidacc/PyStub.py $(srcdir)/../aidacc/tests/testpass.idl -o xgen-$@ \
	; eval "$$TSTDIAGNOSE_LOGTEST"
	@: # check .py generation
	$(Q) mv xgen-$@.py $(TSTOUT)
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testpystub.ref'"
	@: # check .cc generation
	$(Q) mv xgen-$@.cc $(TSTOUT) \
	&& echo "static AIDA_UNUSED PyMethodDef dummy[] = { AIDA_PYSTUB_METHOD_DEFS(), { 0 } };" >>$(TSTOUT) \
	; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testcxxpystub.ref'"
	@: # check .py syntax
	$(Q) $(PYTHON) -c "import py_compile; py_compile.compile ('$(srcdir)/testpystub.ref', 'testpystub.pyc')" \
	&& $(PYTHON) $(srcdir)/testpystub.ref ; eval "$$TSTDIAGNOSE"
	@: # check .cc syntax
	$(Q) cp $(srcdir)/testcxxpystub.ref testcxxpystub.cc
	$(Q) $(CXXCOMPILE) testcxxpystub.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxpystub.cc testcxxpystub.o
CLEANFILES += testcxxpystub.cc testcxxpystub.o testpystub.pyc
check-local: check-PyStub-testpass
# test PyStub files for Rapicorn interfaces
check-rapicorn-idl:
	$(Q) ${AIDACC} $(RAPICORNIDL) > /dev/null ; eval "$$TSTDIAGNOSE"
	$(Q) ${AIDACC} -x $(srcdir)/PyStub $(RAPICORNIDL) -o testmodule ; eval "$$TSTDIAGNOSE"
	$(Q) $(PYTHON) testmodule.py ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testmodule.cc testmodule.py
CLEANFILES += testmodule.cc testmodule.py
check-local: check-rapicorn-idl
