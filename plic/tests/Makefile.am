# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

TESTPLIC = PYTHONPATH="$$PYTHONPATH:$(srcdir)/.." ../plic-@PLIC_VERSION@ $(DEBUG:%=--plic-debug)
PLICINT = ../plic-intern

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

### PLIC check passing tests
check-testpass: $(srcdir)/testpass.idl
	$(Q) ${TESTPLIC} $(srcdir)/testpass.idl >/dev/null ; eval "$$TSTDIAGNOSE"
check-include-p1: $(srcdir)/include-p1.idl
	$(Q) ${TESTPLIC} $(srcdir)/include-p1.idl >/dev/null ; eval "$$TSTDIAGNOSE"
check: check-testpass check-include-p1

### PLIC check failing tests
check-testfail: $(srcdir)/testfail.idl
	$(Q) ${TESTPLIC} --plic-fail-file-test $(srcdir)/testfail.idl >$(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testfail.ref'" "'$(TSTOUT)'"
check: check-testfail

check-plic-standard-types: ptptest
	$(Q) ./ptptest --tests
check: check-plic-standard-types

check-plic-format-TypeMap: ptptest $(srcdir)/testpass.idl
	$(Q) ${TESTPLIC} $(srcdir)/testpass.idl -G TypeMap  ; eval "$$TSTDIAGNOSE" "'plic -G TypeMap'"
	$(Q) ./ptptest plic.out $(TESTPASS_AUXTESTS) >/dev/null ; eval "$$TSTDIAGNOSE" "'ptptest aux-tests'"
	$(Q) rm -f plic.out
check: check-plic-format-TypeMap

check-plic-format-Rapicorn: $(srcdir)/testpass.idl
	$(Q) ${TESTPLIC} $(srcdir)/testpass.idl -G Rapicorn -o $(TSTOUT) ; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/testrapicorn.ref'" "'$(TSTOUT)'"
check: check-plic-format-Rapicorn
