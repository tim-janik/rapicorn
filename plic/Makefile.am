# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

SUBDIRS = . tests

PLICV       = plic-@PLIC_VERSION@
PLICL       = plic

bin_SCRIPTS = ${PLICV}
CLEANFILES += ${PLICV}

YAPPS = ${PYTHON} ${top_srcdir}/yapps2_deb/yapps2.py

# build parser
Parser.py: Parser.g
	${YAPPS} Parser.g xgen-$(@F) 2>&1 | tee xgen-$(@F).errors
	@! grep -q '.' xgen-$(@F).errors			# catch all yapps errors & warnings
	sed <xgen-$(@F) >$@			\
	    -e 's/^from yapps import runtime$$//'		# parser.g has all imports
	rm -f xgen-$(@F) xgen-$(@F).errors
EXTRA_DIST += Parser.g Parser.py
MAINTAINERCLEANFILES += Parser.py

CONFIGVARS = \
	"pyutilsdir"	: "${pyutilsdir}",	\
	"PLIC_VERSION"	: "${VERSION}"

# build plic executable
${PLICV}: plic.py Parser.py
	$(_@) sed <$< >xgen-$(@F)			\
	  -e '1,1s|#!/usr/bin/env python\([0-9]\+\(\.[0-9]\+\)\?\)\?|#!${PYTHON}|' \
	  -e '1,24s|^ *#@PKGINSTALL_CONFIGVARS_IN24LINES@|  ${CONFIGVARS}|'
	$(_@) chmod +x xgen-$(@F)
	$(_@) cp -p xgen-$(@F) $@
	$(_@) rm -f xgen-$(@F) xgen-$(@F).errors
EXTRA_DIST += plic.py

# install plic support module yapps2runtime.py
pyutilsdir     = $(libdir)/${PLICV}/pyutils
plicbackends   = PrettyDump.py TypePackage.py GType.py
pyutils_PYTHON = Decls.py Parser.py AuxData.py yapps2runtime.py ${plicbackends} PlicTypePackage.cc

# tests
TESTPLIC = PYTHONPATH="$$PYTHONPATH:$(srcdir)" ./${PLICV}
installcheck-local:
	$(_@)  ${TESTPLIC} --cc-type-package-parser | grep -q struct
check-outputs:
	$(_@) ${TESTPLIC} --list-formats | grep -q PLIC
check-local: check-outputs

# install/uninstall a "plic" link to "plic-PLIC_VERSION"
install-exec-hook:
	$(_@) cd $(DESTDIR)$(bindir)			\
	  && test -x ${PLICL}			\
	  || $(LN_S) ${PLICV} ${PLICL}
uninstall-hook:	# runs after ${PLICV} was uninstalled
	$(_@) cd $(DESTDIR)$(bindir)			\
	  && test ! -L ${PLICL} -o -x ${PLICL}	\
	  || rm -f ${PLICL}
