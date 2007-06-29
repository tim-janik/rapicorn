# plic - Pluggable IDL Compiler
#
## GNU Lesser General Public License version 2 or any later version.

srcdir=.

all:	plic.py ChangeLog
	$(MAKE) $(AM_MAKEFLAGS) check

plic.py: plic.g
	yapps plic.g xgen-$(@F) 2>&1 | tee xgen-$(@F).errors
	@! grep -q '.' xgen-$(@F).errors	# catch all yapps errors & warnings
	sed 's/^from yapps import runtime$$//' -i xgen-$(@F)	# plic.g has all imports
	chmod +x xgen-$(@F)
	cp xgen-$(@F) $@
	rm -f xgen-$(@F) xgen-$(@F).errors

check:	check-pass check-fail

check-pass:	plic.py testpass.idl
	./plic.py $(srcdir)/testpass.idl

check-fail: plic.py testfail.idl
	echo -n "testfail.idl: checked:" ; \
	egrep -vn '^(//.*|\s*)$$' $(srcdir)/testfail.idl | sed 's/:/ /' | \
	  while read num line ; do \
	    echo -e "namespace X {\n $$line \n}" > xgen-testfail.idl \
	    && ! $(srcdir)/plic.py xgen-testfail.idl > /dev/null 2>&1 \
	    || { echo -e "\ntestfail.idl:$$num: no error: $$line" ; exit 1 ; } ; \
	    echo -n " $$num" ; \
	  done ; echo
	rm -f xgen-testfail.idl

clean:
	rm -f xgen-$(@F) plic.py ChangeLog

# === ChangeLog ===
ChangeLog:	$(shell ls "$${GIT_DIR=.git}/`git-symbolic-ref HEAD`" 2>/dev/null )
	git-log --pretty='format:%ad %an	# %H (%cn)%n%n%s%n%n%b' > xgen-$(@F)
	sed 's/^/	/;s/^	//;/^[ 	]*<unknown>$$/d'           -i xgen-$(@F)
	cp xgen-$(@F) $@ && rm -f xgen-$(@F)
