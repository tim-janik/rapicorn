# Aida - Abstract Interface Definition Architecture
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) # -I$(top_builddir)

AIDACC = $(AIDACC_NODEBUG) $(if $(findstring 1, $(V)), --aida-debug)
AIDACC_NODEBUG = ../aidacc-intern
AIDACC_INSTALLED = PATH="$(bindir):$$PATH" aidacc-@RAPICORN_RELEASE@
TYPEMAP_PY  = $(top_srcdir)/aidacc/TypeMap.py

noinst_PROGRAMS = ptptest
ptptest_SOURCES = ptptest.cc
ptptest_LDFLAGS = -pthread -lpthread
ptptest_LDADD   = $(top_builddir)/rcore/librapicorncore@RAPICORN_RELEASE@.la
$(srcdir)/ptptest.cc: typecodetests.typ
EXTRA_DIST += typecodetests.idl
CLEANFILES += typecodetests.typ

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
	$(Q) ${AIDACC_NODEBUG} --aida-fail-file-test $(srcdir)/testfail.idl >$(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) sed 's/\(:[0-9]\+:[0-9]\+: Trying to find one of \).*/\1.../' <$(TSTOUT) >$(TSTOUT).tmp && mv $(TSTOUT).tmp $(TSTOUT)
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testfail.ref'" "'$(TSTOUT)'"
check-local: check-testfail

# == Aida Unit Tests ==
check-aida-standard-types: ptptest
	$(Q) ./ptptest --tests
check-local: check-aida-standard-types

# == Test TypeMap ==
check-aida-format-TypeMap: $(srcdir)/testpass.idl $(TYPEMAP_PY) ptptest
	$(Q) ${AIDACC} $< -x TypeMap ; eval "$$TSTDIAGNOSE" "'aidacc -x TypeMap'"
	$(Q) ./ptptest idltypes.map $(TESTPASS_AUXTESTS) >/dev/null ; eval "$$TSTDIAGNOSE" "'ptptest aux-tests'"
	$(Q) rm -f idltypes.map
check-local: check-aida-format-TypeMap

# == Test CxxStub Generation for Clinet & Server ==
check-CxxStub-server-gen: $(srcdir)/testcxxserver.ref
	$(Q) ${AIDACC} -x CxxStub -G iface-prefix=I_ -G serverhh -G servercc -G strip-path=$(abs_top_srcdir)/ \
	  $(srcdir)/testpass.idl -o $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testcxxserver.ref'"
check-CxxStub-client-gen: $(srcdir)/testcxxclient.ref
	$(Q) ${AIDACC} -x CxxStub -G iface-prefix=I_ -G clienthh -G clientcc -G strip-path=$(abs_top_srcdir)/ \
	  $(srcdir)/testpass.idl -o $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testcxxclient.ref'"
check-CxxStub-skeleton-gen: $(srcdir)/testcxxskels.ref
	$(Q) ${AIDACC} -x CxxStub -G iface-prefix=I_ -G serverhh -G server-skel -G strip-path=$(abs_top_srcdir)/ \
	  $(srcdir)/testpass.idl -o - > $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testcxxskels.ref'"
check: check-CxxStub-server-gen check-CxxStub-client-gen check-CxxStub-skeleton-gen
EXTRA_DIST += testcxxserver.ref testcxxclient.ref testcxxskels.ref

# == Test CxxStub Code Compilation ==
check-CxxStub-server-code: $(srcdir)/testcxxserver.ref check-CxxStub-client-gen
	$(Q) ${AIDACC} -x CxxStub -G iface-prefix=I_ -G clienthh \
	  $(srcdir)/testpass.idl -o testcxxserver-stub.hh
	$(Q) echo '#include "testcxxserver-stub.hh"'	 >testcxxserver.cc
	$(Q) cat $(srcdir)/testcxxserver.ref		>>testcxxserver.cc
	$(Q) $(CXXCOMPILE) -D__RAPICORN_BUILD__  testcxxserver.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxserver.cc testcxxserver.o testcxxserver-stub.hh
check-CxxStub-client-code: $(srcdir)/testcxxclient.ref
	$(Q) cp $(srcdir)/testcxxclient.ref testcxxclient.cc
	$(Q) $(CXXCOMPILE) testcxxclient.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxclient.cc testcxxclient.o
check-CxxStub-skeleton-code: $(srcdir)/testcxxskels.ref
	$(Q) ${AIDACC} -x CxxStub -G iface-prefix=I_ -G clienthh \
	  $(srcdir)/testpass.idl -o testcxxsrvskel-stub.hh
	$(Q) echo '#include "testcxxsrvskel-stub.hh"'	 >testcxxskels.cc
	$(Q) cat $(srcdir)/testcxxskels.ref		>>testcxxskels.cc
	$(Q) $(CXXCOMPILE) testcxxskels.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxskels.cc testcxxskels.o testcxxsrvskel-stub.hh
check: check-CxxStub-server-code check-CxxStub-client-code check-CxxStub-skeleton-code
