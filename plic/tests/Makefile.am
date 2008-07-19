# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

PLIC = $(top_builddir)/plic/plic-@PLIC_VERSION@

noinst_PROGRAMS = ptptest
ptptest_SOURCES = ptptest.cc

# tests for auxillary data in testpass.idl
TESTPASS_AUXTESTS = --aux-test label=auxtest1 --aux-test auxtest2 --aux-test auxtest3 \
		    --aux-test default=10007 --aux-test hints=rw

check-ptp: ptptest
	${PLIC} $(srcdir)/testpass.idl -O TypePackage
	./ptptest plic.out $(TESTPASS_AUXTESTS) > /dev/null
	rm -f plic.out

installcheck-local: $(srcdir)/testpass.idl
	plic-@PLIC_VERSION@ $(srcdir)/testpass.idl > /dev/null
	plic-@PLIC_VERSION@ --list-formats > /dev/null

check:	check-pass check-fail check-ptp

check-pass:	${PLIC} testpass.idl
	${PLIC} $(srcdir)/testpass.idl > /dev/null

check-fail:	${PLIC} testfail.idl
	echo -n "testfail.idl: checked:" ; \
	egrep -vn '^(//.*|\s*)$$' $(srcdir)/testfail.idl | sed 's/:/ /' | \
	  while read num line ; do \
	    echo -e "namespace X {\n $$line \n}" > xgen-testfail.idl \
	      && ! ${PLIC} xgen-testfail.idl > /dev/null 2>&1 \
	      || { echo -e "\ntestfail.idl:$$num: uncaught error: $$line" ; exit 1 ; } ; \
	    echo -n " $$num" ; \
	  done || exit 1 ; echo
	rm -f xgen-testfail.idl

clean:
	rm -f xgen-$(@F)
