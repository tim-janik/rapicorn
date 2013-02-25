# Rapicorn Rope
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

INCLUDES   += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS       += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS = $(RAPICORN_ROPE_CFLAGS) $(RAPICORN_GUI_CFLAGS) $(PYTHON_CPPFLAGS)

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
cpy2rope.cc: $(RAPICORNIDL) $(srcdir)/CxxPyStub.py
	: # compile IDL definitions into cpy2rope.cc
	${AIDACC} -x $(srcdir)/CxxPyStub $(RAPICORNIDL) -o xgen-$(@F)
	mv xgen-$(@F) $@
$(srcdir)/py-rope.cc: cpy2rope.cc
CLEANFILES += xgen-cpy2rope.cc cpy2rope.cc

# === py2cpy.py ===
py2cpy.py: $(RAPICORNIDL) $(srcdir)/PyStub.py
	${AIDACC} -x $(srcdir)/PyStub $(RAPICORNIDL) -o xgen-$(@F)
	mv xgen-$(@F) $@
	rm -f xgen-$(@F)
CLEANFILES += xgen-py2cpy.py py2cpy.py

# === TESTS ===
# test PyStub
EXTRA_DIST += testpystub.ref PyStub.py
CLEANFILES += testpystub.pyc
check-PyStub-gen: $(srcdir)/testpystub.ref
	$(Q) ${AIDACC} -x $(srcdir)/PyStub $(srcdir)/../aidacc/tests/testpass.idl -o $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testpystub.ref'"
check-PyStub-syntax: $(srcdir)/testpystub.ref
	$(Q) $(PYTHON) -c "import py_compile; py_compile.compile ('$(srcdir)/testpystub.ref', 'testpystub.pyc')" && \
	  $(PYTHON) $(srcdir)/testpystub.ref ; eval "$$TSTDIAGNOSE"
check-local: check-PyStub-gen check-PyStub-syntax
# test CxxPyStub
EXTRA_DIST += CxxPyStub.py testcxxpystub.ref
check-CxxPyStub-gen: $(srcdir)/testcxxpystub.ref
	$(Q) ${AIDACC} -x $(srcdir)/CxxPyStub $(srcdir)/../aidacc/tests/testpass.idl -o $(TSTOUT) \
	&& echo "static AIDA_UNUSED PyMethodDef dummy[] = { AIDA_PYSTUB_METHOD_DEFS(), { 0 } };" >>$(TSTOUT) \
	; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testcxxpystub.ref'"
check-CxxPyStub-syntax: $(srcdir)/testcxxpystub.ref
	$(Q) cp $(srcdir)/testcxxpystub.ref testcxxpystub.cc
	$(Q) $(CXXCOMPILE) testcxxpystub.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxpystub.cc testcxxpystub.o
CLEANFILES += testcxxpystub.cc
check-local: check-CxxPyStub-gen check-CxxPyStub-syntax
# test interfaces.idl with Aida
check-rapicorn-idl:
	$(Q) ${AIDACC} $(RAPICORNIDL) > /dev/null ; eval "$$TSTDIAGNOSE"
check-local: check-rapicorn-idl
# test PyStub with interfaces.idl
check-PyStub-for-interfaces.idl:
	$(Q) ${AIDACC} -x $(srcdir)/PyStub $(RAPICORNIDL) | $(PYTHON) ; eval "$$TSTDIAGNOSE"
check-local: check-PyStub-for-interfaces.idl
