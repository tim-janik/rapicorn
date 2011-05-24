# Rapicorn Rope
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. \
	    $(RAPICORN_ROPE_CFLAGS) $(RAPICORN_GUI_CFLAGS) $(PYTHON_CPPFLAGS)
DEFS      = -DRAPICORN_INTERNALS -DRAPICORN_LOG_DOMAIN=\"RAPICORN-ROPE\" # "
PLICINT   = $(top_builddir)/plic/plic-intern
RAPICORNIDL  = $(srcdir)/../ui/rapicorn.idl
RAPICORNINS  = $(srcdir)/../ui/insertions.cc
HARDCODEDIDL = $(srcdir)/../ui/hardcoded.idl

# === CxxRapicorn ===
noinst_PROGRAMS     = cxxrapicorn
cxxrapicorn_SOURCES = cxx-rope.cc
cxxrapicorn_LDADD   = $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la
EXTRA_DIST         += cxx-rope.hh cxx-client.hh

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
	${PLICINT} -G $(srcdir)/CxxPyStub $(RAPICORNIDL) -o xgen-$(@F)
	mv xgen-$(@F) $@
$(srcdir)/py-rope.cc: cpy2rope.cc
CLEANFILES += xgen-cpy2rope.cc cpy2rope.cc

# === cxx-client ===
cxx-client.hh: $(RAPICORNIDL) $(HARDCODEDIDL) $(RAPICORNINS) $(srcdir)/CxxCaller.py
	: # build cxx-client.hh from rapicorn.idl
	: # ${PLICINT} -G $(srcdir)/CxxCaller -g clienthh $(HARDCODEDIDL) -o xgen-$(@F)
	${PLICINT} -G $(srcdir)/CxxCaller -g clienthh $(RAPICORNIDL) -g shortalias \
	  --insertions $(RAPICORNINS) -o xgen-$(@F)
	mv xgen-$(@F) $@
cxx-client.cc: $(RAPICORNIDL) $(HARDCODEDIDL) $(srcdir)/CxxCaller.py
	: # build cxx-client.cc from rapicorn.idl
	: # ${PLICINT} -G $(srcdir)/CxxCaller -g clientcc $(HARDCODEDIDL) --inclusions '"cxx-client.hh"' -o xgen-$(@F)
	${PLICINT} -G $(srcdir)/CxxCaller -g clientcc $(RAPICORNIDL) --inclusions '"cxx-client.hh"' -o - > xgen-$(@F)
	mv xgen-$(@F) $@
$(srcdir)/cxx-rope.cc: cxx-client.hh cxx-client.cc
CLEANFILES += xgen-cxx-client.hh xgen-cxx-client.cc cxx-client.hh cxx-client.cc

# === py2cpy.py ===
py2cpy.py: $(RAPICORNIDL) $(srcdir)/PyStub.py
	${PLICINT} -G $(srcdir)/PyStub $(RAPICORNIDL) -o xgen-$(@F)
	mv xgen-$(@F) $@
	rm -f xgen-$(@F)
CLEANFILES += xgen-py2cpy.py py2cpy.py

# === TESTS ===
# test PyStub
EXTRA_DIST += testpystub.ref PyStub.py
CLEANFILES += testpystub.pyc
check-PyStub-gen: $(srcdir)/testpystub.ref
	$(Q) ${PLICINT} -G $(srcdir)/PyStub $(srcdir)/../plic/tests/testpass.idl -o $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTVERIFY" "'$(srcdir)/testpystub.ref'"
check-PyStub-syntax: $(srcdir)/testpystub.ref
	$(Q) $(PYTHON) -c "import py_compile; py_compile.compile ('$(srcdir)/testpystub.ref', 'testpystub.pyc')" && \
	  $(PYTHON) $(srcdir)/testpystub.ref ; eval "$$TSTDIAGNOSE"
check-local: check-PyStub-gen check-PyStub-syntax
# test CxxPyStub
EXTRA_DIST += CxxPyStub.py testcxxpystub.ref
check-CxxPyStub-gen: $(srcdir)/testcxxpystub.ref
	$(Q) ${PLICINT} -G $(srcdir)/CxxPyStub $(srcdir)/../plic/tests/testpass.idl -o $(TSTOUT) \
	&& echo "static PLIC_UNUSED PyMethodDef dummy[] = { PLIC_PYSTUB_METHOD_DEFS(), { 0 } };" >>$(TSTOUT) \
	; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTVERIFY" "'$(srcdir)/testcxxpystub.ref'"
check-CxxPyStub-syntax: $(srcdir)/testcxxpystub.ref
	$(Q) cp $(srcdir)/testcxxpystub.ref testcxxpystub.cc
	$(Q) $(CXXCOMPILE) testcxxpystub.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxpystub.cc testcxxpystub.o
check-local: check-CxxPyStub-gen check-CxxPyStub-syntax
# test CxxCaller server and client
check-CxxCaller-server-gen: $(srcdir)/testcxxserver.ref
	$(Q) ${PLICINT} -G $(srcdir)/CxxCaller -g iface-prefix=I_ -g serverhh -g servercc \
	  $(srcdir)/../plic/tests/testpass.idl -o $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTVERIFY" "'$(srcdir)/testcxxserver.ref'"
check-CxxCaller-client-gen: $(srcdir)/testcxxclient.ref
	$(Q) ${PLICINT} -G $(srcdir)/CxxCaller -g iface-prefix=I_ -g clienthh -g clientcc \
	  $(srcdir)/../plic/tests/testpass.idl -o $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTVERIFY" "'$(srcdir)/testcxxclient.ref'"
check-CxxCaller-skeleton-gen: $(srcdir)/testcxxskels.ref
	$(Q) echo -e "#include <rapicorn.hh>\n" > $(TSTOUT)
	$(Q) ${PLICINT} -G $(srcdir)/CxxCaller -g iface-prefix=I_ -g serverhh -g server-skel \
	  $(srcdir)/../plic/tests/testpass.idl -o - >> $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTVERIFY" "'$(srcdir)/testcxxskels.ref'"
check-CxxCaller-server-code: $(srcdir)/testcxxserver.ref
	$(Q) cp $(srcdir)/testcxxserver.ref testcxxserver.cc
	$(Q) $(CXXCOMPILE) testcxxserver.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxserver.cc testcxxserver.o
check-CxxCaller-client-code: $(srcdir)/testcxxclient.ref
	$(Q) cp $(srcdir)/testcxxclient.ref testcxxclient.cc
	$(Q) $(CXXCOMPILE) testcxxclient.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxclient.cc testcxxclient.o
check-CxxCaller-skeleton-code: $(srcdir)/testcxxskels.ref
	$(Q) cp $(srcdir)/testcxxskels.ref testcxxskels.cc
	$(Q) $(CXXCOMPILE) testcxxskels.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxskels.cc testcxxskels.o
EXTRA_DIST += CxxCaller.py testcxxserver.ref testcxxclient.ref testcxxskels.ref
check-local: check-CxxCaller-server-gen check-CxxCaller-client-gen check-CxxCaller-skeleton-gen
check-local: check-CxxCaller-server-code check-CxxCaller-client-code check-CxxCaller-skeleton-code
# test rapicorn.idl with PLIC
check-rapicorn-idl:
	$(Q) ${PLICINT} $(RAPICORNIDL) > /dev/null ; eval "$$TSTDIAGNOSE"
check-local: check-rapicorn-idl
# test PyStub with rapicorn.idl
check-PyStub-for-rapicorn.idl:
	$(Q) ${PLICINT} -G $(srcdir)/PyStub $(RAPICORNIDL) | $(PYTHON) ; eval "$$TSTDIAGNOSE"
check-local: check-PyStub-for-rapicorn.idl
