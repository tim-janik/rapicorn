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

check:	plic.py test.idl
	$(srcdir)/plic.py test.idl

clean:
	rm -f xgen-$(@F) plic.py ChangeLog

# === ChangeLog ===
ChangeLog:	$(shell ls "$${GIT_DIR=.git}/`git-symbolic-ref HEAD`" 2>/dev/null )
	git-log --pretty='format:%ad %an	#%m %cn: %H%n%n%s%n%n%b' > xgen-$(@F)
	sed 's/^/	/;s/^	//;/^[ 	]*<unknown>$$/d'           -i xgen-$(@F)
	cp xgen-$(@F) $@ && rm -f xgen-$(@F)
