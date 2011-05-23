# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

TESTPLIC = PYTHONPATH="$$PYTHONPATH:$(srcdir)/.." ../plic-@PLIC_VERSION@ $(DEBUG:%=--plic-debug)

noinst_PROGRAMS = ptptest
ptptest_SOURCES = ptptest.cc

EXTRA_DIST += testpass.idl testfail.idl include-f1.idl include-f2.idl include-p1.idl include-p2.idl
EXTRA_DIST += testfail.ref testrapicorn.ref
DEBUG	    =

# tests for auxillary data in testpass.idl
TESTPASS_AUXTESTS = --aux-test label=auxtest1 --aux-test auxtest2 --aux-test auxtest3 \
		    --aux-test default=10007 --aux-test hints=rw

installcheck-local: $(srcdir)/testpass.idl
	@$(call TSTTITLE, plic-@PLIC_VERSION@ --cc-type-package-parser)
	$(Q) plic-@PLIC_VERSION@ --cc-type-package-parser | grep -q struct ; $(call TSTRESULT)
	@$(call TSTTITLE, plic-@PLIC_VERSION@ --list-formats)
	$(Q) plic-@PLIC_VERSION@ --list-formats > /dev/null ; $(call TSTRESULT)
	@$(call TSTTITLE, plic-@PLIC_VERSION@ testpass.idl)
	$(Q) plic-@PLIC_VERSION@ $(srcdir)/testpass.idl > /dev/null ; $(call TSTRESULT)


check-pass:	testpass.idl
	@$(call TSTTITLE, check testpass.idl)
	$(Q) ${TESTPLIC} $(srcdir)/testpass.idl > /dev/null ; $(call TSTRESULT)
	@$(call TSTTITLE, check include-p1.idl)
	$(Q) ${TESTPLIC} $(srcdir)/include-p1.idl > /dev/null ; $(call TSTRESULT)

check-fail:	testfail.idl
	@$(call TSTTITLE)
	$(Q) ${TESTPLIC} --plic-fail-file-test $(srcdir)/testfail.idl > $(TSTOUT)
	@$(call TSTCMP, $(srcdir)/testfail.ref)

check-TypePackage: ptptest
	@$(call TSTTITLE)
	$(Q) ${TESTPLIC} $(srcdir)/testpass.idl -G TypePackage
	$(Q) ./ptptest plic.out $(TESTPASS_AUXTESTS) > /dev/null ; $(call TSTRESULT)
	$(Q) rm -f plic.out

check-Rapicorn:
	@$(call TSTTITLE)
	$(Q) ${TESTPLIC} $(srcdir)/testpass.idl -G Rapicorn -o $(TSTOUT)
	@$(call TSTCMP, $(srcdir)/testrapicorn.ref)

check:	check-pass check-fail check-TypePackage check-Rapicorn
