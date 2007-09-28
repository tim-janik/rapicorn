# plic - Pluggable IDL Compiler
# Copyright (C) 2007 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = . tests

PLICV       = plic-@LT_RELEASE@
PLICL       = plic

bin_PROGRAMS = ${PLICV}

plic-@LT_RELEASE@: plic.g
	yapps plic.g xgen-$(@F) 2>&1 | tee xgen-$(@F).errors
	@! grep -q '.' xgen-$(@F).errors			# catch all yapps errors & warnings
	sed 's/^from yapps import runtime$$//' -i xgen-$(@F)	# plic.g has all imports
	chmod +x xgen-$(@F)
	cp -p xgen-$(@F) $@
	rm -f xgen-$(@F) xgen-$(@F).errors
EXTRA_DIST += plic.g

# install/uninstall a "plic" link to "plic-VERSION"
install-exec-hook:
	cd $(DESTDIR)$(bindir)			\
	&& test -x ${PLICL}			\
	|| $(LN_S) ${PLICV} ${PLICL}
uninstall-hook:	# runs after ${PLICV} was uninstalled
	cd $(DESTDIR)$(bindir)			\
	&& test ! -L ${PLICL} -o -x ${PLICL}	\
	|| rm -f ${PLICL}
