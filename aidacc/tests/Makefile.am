# Aida - Abstract Interface Definition Architecture
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) # -I$(top_builddir)
DEFS        += -DRAPICORN_CONVENIENCE

AIDACC = $(AIDACC_NODEBUG) $(if $(findstring 1, $(V)), --aida-debug)
AIDACC_NODEBUG = ../aidacc-intern
AIDACC_INSTALLED = PATH="$(bindir):$$PATH" aidacc
CXXSTUB_PY  = $(top_srcdir)/aidacc/CxxStub.py
TYPEMAP_PY  = $(top_srcdir)/aidacc/TypeMap.py

noinst_PROGRAMS  = $(TEST_PROGS)
LDADDS           = $(top_builddir)/rcore/librapicorncore-@MAJOR@.la

# automake doesn't always emit CXXCOMPILE
CXXCOMPILE ?= $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS)

EXTRA_DIST += typecodetests.idl
typecodetests-api.cc: $(srcdir)/typecodetests.idl $(AIDACC_NODEBUG) $(CXXSTUB_PY)
	$(AM_V_GEN)
	$(Q) ${AIDACC} -x CxxStub -G clienthh $< -o xgen-$(@F).hh
	$(Q) ${AIDACC} -x CxxStub -G clientcc $< -o xgen-$(@F).cc
	$(Q) cat xgen-$(@F).hh xgen-$(@F).cc >$@
	$(Q) rm -f xgen-$(@F).hh xgen-$(@F).cc
CLEANFILES += typecodetests.typ typecodetests-api.cc

EXTRA_DIST += testpass.idl testfail.idl include-f1.idl include-f2.idl include-p1.idl include-p2.idl
EXTRA_DIST += testfail.ref
DEBUG	    =

# tests for auxillary data in testpass.idl
TESTPASS_AUXTESTS = --aux-test label=AuxF64 --aux-test AuxStr --aux-test Aux32 \
		    --aux-test default=10007 --aux-test hints=rw

### Aida build rule for binary type maps
%.typ: %.idl $(TYPEMAP_PY)
	$(AM_V_GEN)
	$(Q) ${AIDACC} $< -x TypeMap -o xtmp-$(@F)
	$(Q) rm -f $@ && mv xtmp-$(@F) $@

### Aida installcheck
bin-aida-cc-type-package-parser:
	$(Q) ${AIDACC_INSTALLED} --cc-type-package-parser | grep -q struct ; eval "$$TSTDIAGNOSE"
installcheck-local: #bin-aida-cc-type-package-parser
bin-aida-list-formats:
	$(Q) ${AIDACC_INSTALLED} --list-formats >/dev/null ; eval "$$TSTDIAGNOSE"
installcheck-local: bin-aida-list-formats
bin-aida-testpass: $(srcdir)/testpass.idl
	$(Q) ${AIDACC_INSTALLED} $(srcdir)/testpass.idl >/dev/null ; eval "$$TSTDIAGNOSE"
installcheck-local: bin-aida-testpass

# == IDL Parsing Tests ==
check-testpass: $(srcdir)/testpass.idl
	$(Q) ${AIDACC} $(srcdir)/testpass.idl >/dev/null ; eval "$$TSTDIAGNOSE"
check-include-p1: $(srcdir)/include-p1.idl
	$(Q) ${AIDACC} $(srcdir)/include-p1.idl >/dev/null ; eval "$$TSTDIAGNOSE"
check-local: check-testpass check-include-p1

# == IDL Error Tests ==
check-testfail: $(srcdir)/testfail.idl
	$(Q) ${AIDACC_NODEBUG} --aida-fail-file-test $(srcdir)/testfail.idl >$(TSTOUT) ; eval "$$TSTDIAGNOSE"
	$(Q) sed 's/\(:[0-9]\+:[0-9]\+: Trying to find one of \).*/\1.../' <$(TSTOUT) >$(TSTOUT).tmp && mv $(TSTOUT).tmp $(TSTOUT)
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testfail.ref'" "'$(TSTOUT)'"
check-local: check-testfail

# == Test CxxStub Generation for Clinet & Server ==
check-CxxStub-server-gen: $(srcdir)/testcxxserver.ref
	$(Q) ${AIDACC} -x CxxStub -G iface-prefix=I_ -G serverhh -G servercc -G aidaids -G strip-path=$(abs_top_srcdir)/ \
	  $(srcdir)/testpass.idl -o $(TSTOUT) ; eval "$$TSTDIAGNOSE"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testcxxserver.ref'"
check-CxxStub-client-gen: $(srcdir)/testcxxclient.ref
	$(Q) ${AIDACC} -x CxxStub -G iface-prefix=I_ -G clienthh -G clientcc -G strip-path=$(abs_top_srcdir)/ \
	  $(srcdir)/testpass.idl -o $(TSTOUT) ; eval "$$TSTDIAGNOSE"
	$(Q) sed -i -e '1i#define _(string) string\' $(TSTOUT)				# '''''
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testcxxclient.ref'"
check: check-CxxStub-server-gen check-CxxStub-client-gen
EXTRA_DIST += testcxxserver.ref testcxxclient.ref

# == Test CxxStub Code Compilation ==
check-CxxStub-server-code: $(srcdir)/testcxxserver.ref check-CxxStub-client-gen
	$(Q) ${AIDACC} -x CxxStub -G iface-prefix=I_ -G clienthh -G strip-path=$(abs_top_srcdir)/ \
	  $(srcdir)/testpass.idl -o testcxxserver-stub.hh
	$(Q) cat $(srcdir)/testcxxserver.ref		 >testcxxserver.cc
	$(Q) echo '#include "testcxxserver-stub.hh"'	>>testcxxserver.cc
	$(Q) $(CXXCOMPILE) -D__RAPICORN_BUILD__  testcxxserver.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxserver.cc testcxxserver.o testcxxserver-stub.hh
check-CxxStub-client-code: $(srcdir)/testcxxclient.ref
	$(Q) cp $(srcdir)/testcxxclient.ref testcxxclient.cc
	$(Q) $(CXXCOMPILE) testcxxclient.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxclient.cc testcxxclient.o
check: check-CxxStub-server-code check-CxxStub-client-code
