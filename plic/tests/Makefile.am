# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

PLICINT = ../plic-intern $(DEBUG:%=--plic-debug)

noinst_PROGRAMS = ptptest
ptptest_SOURCES = ptptest.cc
ptptest_LDFLAGS = -pthread -lpthread
ptptest.cc: typecodetests.typ

EXTRA_DIST += testpass.idl testfail.idl include-f1.idl include-f2.idl include-p1.idl include-p2.idl
EXTRA_DIST += testfail.ref testrapicorn.ref
DEBUG	    =

# tests for auxillary data in testpass.idl
TESTPASS_AUXTESTS = --aux-test label=auxtest1 --aux-test auxtest2 --aux-test auxtest3 \
		    --aux-test default=10007 --aux-test hints=rw

### Plic build rule for binary type maps
%.typ: %.idl
	$(AM_V_GEN)
	$(Q) ${PLICINT} $< -G TypeMap -o xtmp-$(@F)
	$(Q) rm -f $@ && mv xtmp-$(@F) $@

### PLIC installcheck
bin-plic-cc-type-package-parser:
	$(Q) plic-@PLIC_VERSION@ --cc-type-package-parser | grep -q struct ; eval "$$TSTDIAGNOSE"
installcheck-local: #bin-plic-cc-type-package-parser
bin-plic-list-formats:
	$(Q) plic-@PLIC_VERSION@ --list-formats >/dev/null ; eval "$$TSTDIAGNOSE"
installcheck-local: bin-plic-list-formats
bin-plic-testpass: $(srcdir)/testpass.idl
	$(Q) plic-@PLIC_VERSION@ $(srcdir)/testpass.idl >/dev/null ; eval "$$TSTDIAGNOSE"
installcheck-local: bin-plic-testpass

# == IDL Parsing Tests ==
check-testpass: $(srcdir)/testpass.idl
	$(Q) ${PLICINT} $(srcdir)/testpass.idl >/dev/null ; eval "$$TSTDIAGNOSE"
check-include-p1: $(srcdir)/include-p1.idl
	$(Q) ${PLICINT} $(srcdir)/include-p1.idl >/dev/null ; eval "$$TSTDIAGNOSE"
check: check-testpass check-include-p1

# == IDL Error Tests ==
check-testfail: $(srcdir)/testfail.idl
	$(Q) ${PLICINT} --plic-fail-file-test $(srcdir)/testfail.idl >$(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testfail.ref'" "'$(TSTOUT)'"
check: check-testfail

# == Plic Unit Tests ==
check-plic-standard-types: ptptest
	$(Q) ./ptptest --tests
check: check-plic-standard-types

# == Test TypeMap ==
check-plic-format-TypeMap: ptptest $(srcdir)/testpass.idl
	$(Q) ${PLICINT} $(srcdir)/testpass.idl -G TypeMap  ; eval "$$TSTDIAGNOSE" "'plic -G TypeMap'"
	$(Q) ./ptptest plic.out $(TESTPASS_AUXTESTS) >/dev/null ; eval "$$TSTDIAGNOSE" "'ptptest aux-tests'"
	$(Q) rm -f plic.out
check: check-plic-format-TypeMap

# == Test Rapicorn ==
check-plic-format-Rapicorn: $(srcdir)/testpass.idl
	$(Q) ${PLICINT} $(srcdir)/testpass.idl -G Rapicorn -o $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testrapicorn.ref'" "'$(TSTOUT)'"
check: check-plic-format-Rapicorn

# == Test CxxStub Generation for Clinet & Server ==
check-CxxStub-server-gen: $(srcdir)/testcxxserver.ref
	$(Q) ${PLICINT} -G CxxStub -g iface-prefix=I_ -g serverhh -g servercc -g RapicornSignal \
	  $(srcdir)/testpass.idl -o $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testcxxserver.ref'"
check-CxxStub-client-gen: $(srcdir)/testcxxclient.ref
	$(Q) ${PLICINT} -G CxxStub -g iface-prefix=I_ -g clienthh -g clientcc -g RapicornSignal \
	  $(srcdir)/testpass.idl -o $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testcxxclient.ref'"
check-CxxStub-skeleton-gen: $(srcdir)/testcxxskels.ref
	$(Q) ${PLICINT} -G CxxStub -g iface-prefix=I_ -g serverhh -g server-skel -g RapicornSignal \
	  $(srcdir)/testpass.idl -o - > $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testcxxskels.ref'"
check-local: check-CxxStub-server-gen check-CxxStub-client-gen check-CxxStub-skeleton-gen
EXTRA_DIST += testcxxserver.ref testcxxclient.ref testcxxskels.ref

# == Test CxxStub Code Compilation ==
check-CxxStub-server-code: $(srcdir)/testcxxserver.ref
	$(Q) cp $(srcdir)/testcxxserver.ref testcxxserver.cc
	$(Q) $(CXXCOMPILE) -D__RAPICORN_BUILD__  testcxxserver.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxserver.cc testcxxserver.o
check-CxxStub-client-code: $(srcdir)/testcxxclient.ref
	$(Q) cp $(srcdir)/testcxxclient.ref testcxxclient.cc
	$(Q) $(CXXCOMPILE) testcxxclient.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxclient.cc testcxxclient.o
check-CxxStub-skeleton-code: $(srcdir)/testcxxskels.ref
	$(Q) cp $(srcdir)/testcxxskels.ref testcxxskels.cc
	$(Q) $(CXXCOMPILE) testcxxskels.cc -c ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f testcxxskels.cc testcxxskels.o
check-local: check-CxxStub-server-code check-CxxStub-client-code check-CxxStub-skeleton-code
