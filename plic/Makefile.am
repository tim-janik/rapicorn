# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

SUBDIRS = . tests

PLICV	= plic-@PLIC_VERSION@
PLICINT	= ./plic-intern
BINPLIC	= plic

bin_SCRIPTS    = ${PLICV}
noinst_SCRIPTS = ${PLICINT}
CLEANFILES    += ${PLICINT} ${PLICV}

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

CONFIGINT = "pyutilsdir" : "${abs_srcdir}",	"PLIC_VERSION" : "${VERSION}", "INTERN" : 1
CONFIGBIN = "pyutilsdir" : "${pyutilsdir}",	"PLIC_VERSION" : "${VERSION}"

# build plic executables
${PLICINT}: plic.py Parser.py
	$(AM_V_GEN)
	$(Q) sed <$< >xgen-$(@F)			\
	  -e '1,1s|#!/usr/bin/env python\([0-9]\+\(\.[0-9]\+\)\?\)\?|#!${PYTHON}|' \
	  -e '1,24s|^ *#@PKGINSTALL_CONFIGVARS_IN24LINES@|  ${CONFIGINT}|'
	$(Q) chmod +x xgen-$(@F)
	$(Q) cp -p xgen-$(@F) $@
	$(Q) rm -f xgen-$(@F) xgen-$(@F).errors
EXTRA_DIST += plic.py
${PLICV}: plic.py Parser.py
	$(AM_V_GEN)
	$(Q) sed <$< >xgen-$(@F)			\
	  -e '1,1s|#!/usr/bin/env python\([0-9]\+\(\.[0-9]\+\)\?\)\?|#!${PYTHON}|' \
	  -e '1,24s|^ *#@PKGINSTALL_CONFIGVARS_IN24LINES@|  ${CONFIGBIN}|'
	$(Q) chmod +x xgen-$(@F)
	$(Q) cp -p xgen-$(@F) $@
	$(Q) rm -f xgen-$(@F) xgen-$(@F).errors

# == Plic builtins ==
EXTRA_DIST = builtins.typ
builtins.cc: $(srcdir)/builtins.idl ${PLICINT} plic.py
	$(AM_V_GEN)
	$(Q) ${PLICINT} --system-typedefs $< -G TypeMap -o $(@F).typ
	$(Q) ${PLICINT} --cc-intern-file $(@F).typ > xtmp-$(@F)
	$(Q) cp -p xtmp-$(@F) $@
	$(Q) rm -f $(@F).typ xtmp-$(@F)
$(srcdir)/../rcore/plicutypes.cc: builtins.cc
CLEANFILES += builtins.cc
noinst_DATA = builtins.cc

# install plic support module yapps2runtime.py
pyutilsdir     = $(libdir)/${PLICV}/pyutils
plicbackends   = PrettyDump.py TypeMap.py Rapicorn.py GType.py
pyutils_PYTHON = Decls.py GenUtils.py Parser.py AuxData.py yapps2runtime.py ${plicbackends}

### plic-intern tests
check-plic-intern-cc-type-package-parser:
	$(Q) ${PLICINT} --cc-type-package-parser | grep -q struct ; eval "$$TSTDIAGNOSE"
check-local: check-plic-intern-cc-type-package-parser
check-plic-intern-list-formats:
	$(Q) ${PLICINT} --list-formats | grep -q PLIC ; eval "$$TSTDIAGNOSE"
check-local: check-plic-intern-list-formats
# post-install tests
bin-plic-cc-type-package-parser:
	$(Q) ${PLICV} --cc-type-package-parser | grep -q struct ; eval "$$TSTDIAGNOSE"
installcheck-local: #bin-plic-cc-type-package-parser

# install/uninstall a "plic" link to "plic-PLIC_VERSION"
install-exec-hook:
	$(Q) cd $(DESTDIR)$(bindir)			\
	  && test -x ${BINPLIC}			\
	  || (rm -f ${BINPLIC} && $(LN_S) ${PLICV} ${BINPLIC} )
uninstall-hook:	# runs after ${PLICV} was uninstalled
	$(Q) cd $(DESTDIR)$(bindir)			\
	  && test ! -L ${BINPLIC} -o -x ${BINPLIC}	\
	  || rm -f ${BINPLIC}

clean-local:
	rm -f *.pyc
