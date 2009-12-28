# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

SUBDIRS = . tests

PLICV	= plic-@PLIC_VERSION@
PLICINT	= ./plic-intern
BINPLIC	= plic

bin_SCRIPTS = ${PLICINT} ${PLICV}
CLEANFILES += ${PLICINT} ${PLICV}

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

CONFIGINT = "pyutilsdir" : "${abs_srcdir}",	"PLIC_VERSION" : "${VERSION}"
CONFIGBIN = "pyutilsdir" : "${pyutilsdir}",	"PLIC_VERSION" : "${VERSION}"

# build plic executables
${PLICINT}: plic.py Parser.py
	$(_@) sed <$< >xgen-$(@F)			\
	  -e '1,1s|#!/usr/bin/env python\([0-9]\+\(\.[0-9]\+\)\?\)\?|#!${PYTHON}|' \
	  -e '1,24s|^ *#@PKGINSTALL_CONFIGVARS_IN24LINES@|  ${CONFIGINT}|'
	$(_@) chmod +x xgen-$(@F)
	$(_@) cp -p xgen-$(@F) $@
	$(_@) rm -f xgen-$(@F) xgen-$(@F).errors
EXTRA_DIST += plic.py
${PLICV}: plic.py Parser.py
	$(_@) sed <$< >xgen-$(@F)			\
	  -e '1,1s|#!/usr/bin/env python\([0-9]\+\(\.[0-9]\+\)\?\)\?|#!${PYTHON}|' \
	  -e '1,24s|^ *#@PKGINSTALL_CONFIGVARS_IN24LINES@|  ${CONFIGBIN}|'
	$(_@) chmod +x xgen-$(@F)
	$(_@) cp -p xgen-$(@F) $@
	$(_@) rm -f xgen-$(@F) xgen-$(@F).errors

# install plic support module yapps2runtime.py
pyutilsdir     = $(libdir)/${PLICV}/pyutils
plicbackends   = PrettyDump.py TypePackage.py Rapicorn.py GType.py
pyutils_PYTHON = Decls.py Parser.py AuxData.py yapps2runtime.py ${plicbackends} PlicTypePackage.cc

# tests
check-plic-intern:
	@$(call TSTTITLE, test-cc-parser)
	$(_@) ${PLICINT} --cc-type-package-parser | grep -q struct ; $(call TSTRESULT)
	@$(call TSTTITLE, test-formats)
	$(_@) ${PLICINT} --list-formats | grep -q PLIC ; $(call TSTRESULT)
check-local: check-plic-intern
# post-install tests
installcheck-local:
	$(_@) ${PLICV} --cc-type-package-parser | grep -q struct

# install/uninstall a "plic" link to "plic-PLIC_VERSION"
install-exec-hook:
	$(_@) cd $(DESTDIR)$(bindir)			\
	  && test -x ${BINPLIC}			\
	  || $(LN_S) ${PLICV} ${BINPLIC}
uninstall-hook:	# runs after ${PLICV} was uninstalled
	$(_@) cd $(DESTDIR)$(bindir)			\
	  && test ! -L ${BINPLIC} -o -x ${BINPLIC}	\
	  || rm -f ${BINPLIC}
