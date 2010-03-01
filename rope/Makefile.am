# Rapicorn Rope
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(RAPICORN_ROPE_CFLAGS)
DEFS      = -DRAPICORN_INTERNALS -DRAPICORN_LOG_DOMAIN=\"RAPICORN\" # "
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

# build protocol glue
build-proto-glue:
	rm -f protocol.pb.h protocol.pb.cc
	$(PROTOC) $(srcdir)/protocol.proto --python_out=.
	$(PROTOC) $(srcdir)/protocol.proto --cpp_out=.
	mv protocol.pb.h protocol-pb2.hh
	sed 's/^#include "protocol.pb.h"/#include "protocol-pb2.hh"/' <protocol.pb.cc >protocol-pb2.cc
	rm -f protocol.pb.cc
	$(CXXCOMPILE) protocol-pb2.cc -c $(PROTOBUF_LIBS)
EXTRA_DIST += protocol.proto
check-local: build-proto-glue

# tests
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
