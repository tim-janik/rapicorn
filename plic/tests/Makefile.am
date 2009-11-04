# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

TESTPLIC = PYTHONPATH="$$PYTHONPATH:$(srcdir)/.." ../plic-@PLIC_VERSION@ $(DEBUG:%=--plic-debug)

noinst_PROGRAMS = ptptest
ptptest_SOURCES = ptptest.cc

EXTRA_DIST += testpass.idl testfail.idl
DEBUG	    =

# tests for auxillary data in testpass.idl
TESTPASS_AUXTESTS = --aux-test label=auxtest1 --aux-test auxtest2 --aux-test auxtest3 \
		    --aux-test default=10007 --aux-test hints=rw

installcheck-local: $(srcdir)/testpass.idl
	@$(call TSTTITLE, plic-@PLIC_VERSION@ --cc-type-package-parser)
	$(_@) plic-@PLIC_VERSION@ --cc-type-package-parser | grep -q struct ; $(call TSTRESULT)
	@$(call TSTTITLE, plic-@PLIC_VERSION@ --list-formats)
	$(_@) plic-@PLIC_VERSION@ --list-formats > /dev/null ; $(call TSTRESULT)
	@$(call TSTTITLE, plic-@PLIC_VERSION@ testpass.idl)
	$(_@) plic-@PLIC_VERSION@ $(srcdir)/testpass.idl > /dev/null ; $(call TSTRESULT)


check-pass:	testpass.idl
	@$(call TSTTITLE)
	$(_@) ${TESTPLIC} $(srcdir)/testpass.idl > /dev/null ; $(call TSTRESULT)

check-fail:	testfail.idl
	@$(call TSTTITLE)
	$(_@) ${TESTPLIC} --plic-fail-file-test $(srcdir)/testfail.idl > $(TSTOUT)
	@$(call TSTCMP, testfail.ref)

check-TypePackage: ptptest
	@$(call TSTTITLE)
	$(_@) ${TESTPLIC} $(srcdir)/testpass.idl -O TypePackage
	$(_@) ./ptptest plic.out $(TESTPASS_AUXTESTS) > /dev/null ; $(call TSTRESULT)
	$(_@) rm -f plic.out

check:	check-pass check-fail check-TypePackage
