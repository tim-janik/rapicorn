# plic tests
# Copyright (C) 2007 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

PLIC = $(top_builddir)/plic/plic-@LT_RELEASE@

check:	check-pass check-fail

check-pass:	${PLIC} testpass.idl
	${PLIC} $(srcdir)/testpass.idl

check-fail:	${PLIC} testfail.idl
	echo -n "testfail.idl: checked:" ; \
	egrep -vn '^(//.*|\s*)$$' $(srcdir)/testfail.idl | sed 's/:/ /' | \
	  while read num line ; do \
	    echo -e "namespace X {\n $$line \n}" > xgen-testfail.idl \
	      && ! ${PLIC} xgen-testfail.idl > /dev/null 2>&1 \
	      || { echo -e "\ntestfail.idl:$$num: no error: $$line" ; exit 1 ; } ; \
	    echo -n " $$num" ; \
	  done ; echo
	rm -f xgen-testfail.idl

clean:
	rm -f xgen-$(@F)
