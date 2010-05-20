# Rapicorn Rope
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(RAPICORN_ROPE_CFLAGS) $(PYTHON_CPPFLAGS)
DEFS      = -DRAPICORN_INTERNALS -DRAPICORN_LOG_DOMAIN=\"RAPICORN-ROPE\" # "
PLICINT   = $(top_builddir)/plic/plic-intern
RAPICORNIDL = $(srcdir)/../ui/rapicorn.idl

# === sources ===
pyrapicorn_cc_sources = rope.cc
EXTRA_DIST           += rope.hh

# === Python module ===
pyrapicorndir          = ${pythondir}/@RAPICORN_NAMESPACE_NAME@
pyrapicorn_PYTHON      = __init__.py py2cpy.py
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
$(srcdir)/rope.cc: cpy2rope.cc
CLEANFILES += xgen-cpy2rope.cc cpy2rope.cc

# === rope2cxx.cc ===
rope2cxx.cc: $(RAPICORNIDL) $(srcdir)/CxxCaller.py
	: # compile IDL definitions into rope2cxx.cc
	${PLICINT} -G $(srcdir)/CxxCaller -g server $(RAPICORNIDL) -o xgen-$(@F)
	mv xgen-$(@F) $@
$(srcdir)/rope.cc: rope2cxx.cc
CLEANFILES += xgen-rope2cxx.cc rope2cxx.cc

# === py2cpy.py ===
py2cpy.py: $(RAPICORNIDL) $(srcdir)/PyStub.py
	${PLICINT} -G $(srcdir)/PyStub $(RAPICORNIDL) -o xgen-$(@F)
	(echo "class ReferenceCountable (object): pass" ; cat xgen-$(@F) ) > $@
	rm -f xgen-$(@F)
CLEANFILES += xgen-py2cpy.py py2cpy.py

# === TESTS ===
# test PyStub
EXTRA_DIST += testpystub.ref
check-pystub:
	@$(call TSTTITLE)
	$(_@) ${PLICINT} -G $(srcdir)/PyStub $(srcdir)/../plic/tests/testpass.idl -o $(TSTOUT)
	@$(call TSTCMP, $(srcdir)/testpystub.ref)
	@$(call TSTTITLE,check-pystub-syntax)
	$(_@) $(PYTHON) -c "import py_compile; py_compile.compile ('$(srcdir)/testpystub.ref', 'testpystub.pyc')" && \
	  $(PYTHON) $(srcdir)/testpystub.ref ; $(call TSTRESULT)
EXTRA_DIST += PyStub.py
CLEANFILES += testpystub.pyc
check-local: check-pystub
# test CxxPyStub
check-cxxpystub:
	@$(call TSTTITLE)
	$(_@) ${PLICINT} -G $(srcdir)/CxxPyStub $(srcdir)/../plic/tests/testpass.idl -o $(TSTOUT)
	@$(call TSTCMP, $(srcdir)/testcxxpystub.ref)
	@$(call TSTTITLE,check-cxxpystub-syntax)
	$(_@) cp $(srcdir)/testcxxpystub.ref testcxxpystub.cc
	$(_@) $(CXXCOMPILE) testcxxpystub.cc -c ; $(call TSTRESULT)
	$(_@) rm -f testcxxpystub.cc testcxxpystub.o
EXTRA_DIST += CxxPyStub.py testcxxpystub.ref
check-local: check-cxxpystub
# test CxxCaller
check-cxxcaller:
	@$(call TSTTITLE)
	$(_@) ${PLICINT} -G $(srcdir)/CxxCaller $(srcdir)/../plic/tests/testpass.idl -o $(TSTOUT)
	@$(call TSTCMP, $(srcdir)/testcxxcaller.ref)
	@$(call TSTTITLE, check-cxxcaller-syntax)
	$(_@) cp $(srcdir)/testcxxcaller.ref testcxxcaller.cc
	$(_@) $(CXXCOMPILE) testcxxcaller.cc -c ; $(call TSTRESULT)
	$(_@) rm -f testcxxcaller.cc testcxxcaller.o
EXTRA_DIST += CxxCaller.py testcxxcaller.ref
check-local: check-cxxcaller
# test CxxCaller Skeleton
check-cppskels:
	@$(call TSTTITLE)
	@echo -e "#include <rapicorn.hh>\n" \
	  "class ProtoRecord; class ProtoSequence; class ProtoArg; class ProtoMessage;" \
	  > $(TSTOUT)
	$(_@) ${PLICINT} -G $(srcdir)/CxxCaller -g interface \
	  $(srcdir)/../plic/tests/testpass.idl -o - >> $(TSTOUT)
	$(_@) ${PLICINT} -G $(srcdir)/CxxCaller -g interface-skel \
	  $(srcdir)/../plic/tests/testpass.idl -o - >> $(TSTOUT)
	@$(call TSTCMP, $(srcdir)/testcppskels.ref)
	@$(call TSTTITLE, check-cppskels-syntax)
	$(_@) cp $(srcdir)/testcppskels.ref testcppskels.cc
	$(_@) $(CXXCOMPILE) testcppskels.cc -c ; $(call TSTRESULT)
	$(_@) rm -f testcppskels.cc testcppskels.o
# $(TSTOUT)
EXTRA_DIST += testcppskels.ref
check-local: check-cppskels
# test rapicorn.idl with PLIC
check-rapicorn-idl:
	@$(call TSTTITLE)
	$(_@) ${PLICINT} $(RAPICORNIDL) > /dev/null ; $(call TSTRESULT)
check-local: check-rapicorn-idl
# test PyStub with rapicorn.idl
check-pystub-for-rapicorn.idl:
	@$(call TSTTITLE)
	$(_@) (echo "class ReferenceCountable: pass"; \
	       ${PLICINT} -G $(srcdir)/PyStub $(RAPICORNIDL) ) | $(PYTHON) ; $(call TSTRESULT)
check-local: check-pystub-for-rapicorn.idl
