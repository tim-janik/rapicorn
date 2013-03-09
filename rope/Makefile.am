# Rapicorn Rope
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

INCLUDES   += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS       += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS = $(RAPICORN_ROPE_CFLAGS) $(RAPICORN_GUI_CFLAGS) $(PYTHON_CPPFLAGS)

PYMODULE    = $(RAPICORN_NAMESPACE_NAME)

AIDACC       = $(top_builddir)/aidacc/aidacc-intern
RAPICORNIDL  = $(srcdir)/../ui/interfaces.idl
HARDCODEDIDL = $(srcdir)/../ui/hardcoded.idl

# === CxxRapicorn ===
noinst_PROGRAMS      = cxxrapicorn
cxxrapicorn_SOURCES  = cxx-rope.cc
cxxrapicorn_LDADD    = $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la

# === PyRapicorn ===
pyrapicorn_cc_sources = py-rope.cc
EXTRA_DIST           += py-rope.hh
pyrapicorndir          = ${pythondir}/@RAPICORN_NAMESPACE_NAME@
pyrapicorn_PYTHON      = __init__.py py2cpy.py main.py
pyrapicorn_LTLIBRARIES = pyRapicorn.la
pyRapicorn_la_SOURCES  = $(pyrapicorn_cc_sources)
pyRapicorn_la_LIBADD   = $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la
pyRapicorn_la_LDFLAGS  = $(strip				\
	-module -avoid-version -export-symbols-regex pyRapicorn	\
	-no-undefined $(SYMBOLIC_LDFLAGS)			\
)
$(pyrapicorn_LTLIBRARIES): $(pyrapicorn_PYTHON) # force python source creation
python-call-info:
	@echo 'PYTHONPATH="$$PYTHONPATH:$(pythondir)" python # -c "import @RAPICORN_NAMESPACE_NAME@"'

# === cpy2rope.cc ===
py2cpy.py cpy2rope.cc: $(RAPICORNIDL) $(srcdir)/../aidacc/PyStub.py
	: # compile IDL definitions into cpy2rope.cc
	${AIDACC} -x $(srcdir)/../aidacc/PyStub $(RAPICORNIDL) -o $(PYMODULE)
	@mv $(PYMODULE).cc cpy2rope.cc && mv $(PYMODULE).py py2cpy.py
CLEANFILES += py2cpy.py cpy2rope.cc
$(srcdir)/py-rope.cc: cpy2rope.cc

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
