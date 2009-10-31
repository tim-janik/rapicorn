# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

TESTPLIC = PYTHONPATH="$$PYTHONPATH:$(srcdir)/.." ../plic-@PLIC_VERSION@

noinst_PROGRAMS = ptptest
ptptest_SOURCES = ptptest.cc

EXTRA_DIST += testpass.idl testfail.idl
NOUPDATE    = true

# tests for auxillary data in testpass.idl
TESTPASS_AUXTESTS = --aux-test label=auxtest1 --aux-test auxtest2 --aux-test auxtest3 \
		    --aux-test default=10007 --aux-test hints=rw

check-ptp: ptptest
	${TESTPLIC} $(srcdir)/testpass.idl -O TypePackage
	./ptptest plic.out $(TESTPASS_AUXTESTS) > /dev/null
	rm -f plic.out

installcheck-local: $(srcdir)/testpass.idl
	plic-@PLIC_VERSION@ --cc-type-package-parser | grep -q struct
	plic-@PLIC_VERSION@ --list-formats > /dev/null
	plic-@PLIC_VERSION@ $(srcdir)/testpass.idl > /dev/null


check:	check-pass check-fail check-ptp

check-pass:	testpass.idl
	@TL=`printf %-69s "$(@F):"` && TR=`printf %70s "$$TL"` && echo -n "$$TR"
	@${TESTPLIC} $(srcdir)/testpass.idl > /dev/null && echo OK

check-fail:	testfail.idl
	@TL=`printf %-69s "$(@F):"` && TR=`printf %70s "$$TL"` && echo -n "$$TR"
	@${TESTPLIC} --plic-fail-file-test $(srcdir)/testfail.idl >xtmp-testfail.out
	@${NOUPDATE} || cp -v xtmp-testfail.out testfail.ref
	@cmp --quiet testfail.ref xtmp-testfail.out && echo OK || \
	  { echo FAIL; diff -up testfail.ref xtmp-testfail.out; exit 1; }
	@rm -f xtmp-testfail.out

force-update:
	$(MAKE) NOUPDATE=false check-fail
