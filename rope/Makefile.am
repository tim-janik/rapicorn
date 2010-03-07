# Rapicorn Rope
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(RAPICORN_ROPE_CFLAGS) $(PYTHON_CPPFLAGS)
DEFS      = -DRAPICORN_INTERNALS -DRAPICORN_LOG_DOMAIN=\"RAPICORN-ROPE\" # "
PLICINT   = $(top_builddir)/plic/plic-intern


# === sources ===
rope_public_headers = $(strip		\
	message.hh			\
)
rope_cc_sources = $(strip		\
	message.cc			\
)
rope_private_files = $(strip		\
)
pyrapicorn_cc_sources = $(strip		\
	pyrope.cc			\
)

# === Python module ===
pyrapicorndir          = ${pythondir}/@RAPICORN_NAMESPACE_NAME@
pyrapicorn_PYTHON      = __init__.py
pyrapicorn_LTLIBRARIES = pyRapicorn.la
pyRapicorn_la_SOURCES  = $(pyrapicorn_cc_sources)
pyRapicorn_la_LDFLAGS  = -module -avoid-version -no-undefined -export-symbols-regex pyRapicorn
pyRapicorn_la_LIBADD   = $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la $(PROTOBUF_LIBS)
python-call-info:
	@echo 'PYTHONPATH="$$PYTHONPATH:$(pythondir)" python # -c "import @RAPICORN_NAMESPACE_NAME@"'

# === libraries ===
noinst_LTLIBRARIES                                = librapicornrope@RAPICORN_RELEASE@.la
librapicornrope@RAPICORN_RELEASE@includedir       = $(includedir)/rapicorn@RAPICORN_RELEASE@/rope
librapicornrope@RAPICORN_RELEASE@include_HEADERS  = $(rope_public_headers)
librapicornrope@RAPICORN_RELEASE@_la_SOURCES      = $(rope_cc_sources)
librapicornrope@RAPICORN_RELEASE@_la_DEPENDENCIES = ldscript.map
librapicornrope@RAPICORN_RELEASE@_la_LIBADD       = $(RAPICORN_ROPE_LIBS) -lm
librapicornrope@RAPICORN_RELEASE@_la_LDFLAGS      = $(strip	  \
	-Wl,--version-script=$(srcdir)/ldscript.map -no-undefined \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	  \
) # set libtool version and export symbols for resolving
EXTRA_DIST += $(rope_private_files) $(librapicornrope@RAPICORN_RELEASE@_la_DEPENDENCIES)

# keep this .o rule in sync with the corresponding .la rule from Makefile.in
librapicornrope@RAPICORN_RELEASE@.o: $(librapicornrope@RAPICORN_RELEASE@_la_OBJECTS) $(librapicornrope@RAPICORN_RELEASE@_la_DEPENDENCIES)
	$(CXXLINK) -rpath $(libdir) $(librapicornrope@RAPICORN_RELEASE@_la_LDFLAGS) $(librapicornrope@RAPICORN_RELEASE@_la_OBJECTS) # $(librapicornrope@RAPICORN_RELEASE@_la_LIBADD) $(LIBS)
all-am: librapicornrope@RAPICORN_RELEASE@.o
CLEANFILES += librapicornrope@RAPICORN_RELEASE@.lo librapicornrope@RAPICORN_RELEASE@.o

# === protocol glue ===
EXTRA_DIST += protocol.proto
build-proto-glue:
	$(PROTOC) $(srcdir)/protocol.proto --python_out=.
CLEANFILES += protocol_pb2.py  protocol_pb2.pyc
check-local: build-proto-glue

# === protocol-pb2.[hc][hc] ===
protocol-pb2.hh: $(srcdir)/protocol.proto
	rm -f protocol.pb.h protocol.pb.cc
	$(PROTOC) $(srcdir)/protocol.proto --cpp_out=.
	mv protocol.pb.h protocol-pb2.hh
	sed 's/^#include "protocol.pb.h"/#include "protocol-pb2.hh"/' <protocol.pb.cc >protocol-pb2.cc
	rm -f protocol.pb.cc
protocol-pb2.cc: protocol-pb2.hh
CLEANFILES += protocol.pb.h protocol.pb.cc protocol-pb2.cc protocol-pb2.hh

# === pycstub.cc ===
pycstub.cc: $(srcdir)/PyCStub.py $(srcdir)/base.idl
	: # compile IDL definitions into pycstub.cc
	${PLICINT} -O=$(srcdir)/PyCStub $(srcdir)/base.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
$(srcdir)/pyrope.cc: pycstub.cc
CLEANFILES += xgen-pycstub.cc pycstub.cc

# === tests ===
check-pystub:
	@$(call TSTTITLE)
	$(_@) ${PLICINT} -O=$(srcdir)/PyStub $(srcdir)/../plic/tests/testpass.idl -o $(TSTOUT)
	@$(call TSTCMP, $(srcdir)/testpystub.ref)
	@$(call TSTTITLE,check-pystub-syntax)
	$(_@) $(PYTHON) -c "import py_compile; py_compile.compile ('$(srcdir)/testpystub.ref', 'testpystub.pyc')" && \
	  $(PYTHON) $(srcdir)/testpystub.ref ; $(call TSTRESULT)
#	rm -f testpystub.pyc
EXTRA_DIST += testpystub.ref
check-local: check-pystub

check-pycstub:
	@$(call TSTTITLE)
	$(_@) ${PLICINT} -O=$(srcdir)/PyCStub $(srcdir)/../plic/tests/testpass.idl -o $(TSTOUT)
	@$(call TSTCMP, $(srcdir)/testpycstub.ref)
	@$(call TSTTITLE,check-pycstub-syntax)
	$(_@) cp $(srcdir)/testpycstub.ref testpycstub.cc
	$(_@) $(CXXCOMPILE) testpycstub.cc -c ; $(call TSTRESULT)
	$(_@) rm -f testpycstub.cc testpycstub.o
EXTRA_DIST += testpycstub.ref
check-local: check-pycstub

check-cppstub:
	@$(call TSTTITLE)
	$(_@) ${PLICINT} -O=$(srcdir)/CppStub $(srcdir)/../plic/tests/testpass.idl -o $(TSTOUT)
	@$(call TSTCMP, $(srcdir)/testcppstub.ref)
	@$(call TSTTITLE,check-cppstub-syntax)
	$(_@) cp $(srcdir)/testcppstub.ref testcppstub.cc
	$(_@) $(CXXCOMPILE) testcppstub.cc -c ; $(call TSTRESULT)
	$(_@) rm -f testcppstub.cc testcppstub.o
EXTRA_DIST += testcppstub.ref
check-local: check-cppstub

check-base-idl:
	@$(call TSTTITLE)
	$(_@) ${PLICINT} $(srcdir)/base.idl > /dev/null ; $(call TSTRESULT)
EXTRA_DIST += base.idl
check-local: check-base-idl
check-pystub-for-base.idl:
	@$(call TSTTITLE)
	$(_@) ${PLICINT} -O=$(srcdir)/PyStub $(srcdir)/base.idl | $(PYTHON) ; $(call TSTRESULT)
check-local: check-pystub-for-base.idl
