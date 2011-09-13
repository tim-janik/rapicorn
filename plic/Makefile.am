# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

SUBDIRS = . tests


bin_SCRIPTS         = ${PLICV}
noinst_SCRIPTS      = ${PLICINT}
cxxruntimedir       = $(includedir)/rapicorn@RAPICORN_RELEASE@/plic
cxxruntime_HEADERS  = runtime.hh runtime.cc runtypes.cc
PLICLINK	    = plic			## user visible excutable name
PLICV	            = plic-@PLIC_VERSION@	## installed executable name
PLICINT	            = ./plic-intern		## internal version used during build time
CLEANFILES         += ${PLICINT} ${PLICV}
YAPPS 		    = ${PYTHON} ${top_srcdir}/yapps2_deb/yapps2.py

# == Build Parser ==
Parser.py: Parser.g
	${YAPPS} Parser.g xgen-$(@F) 2>&1 | tee xgen-$(@F).errors
	@! grep -q '.' xgen-$(@F).errors			# catch all yapps errors & warnings
	sed <xgen-$(@F) >$@			\
	    -e 's/^from yapps import runtime$$//'		# parser.g has all imports
	rm -f xgen-$(@F) xgen-$(@F).errors
EXTRA_DIST += Parser.g Parser.py
MAINTAINERCLEANFILES += Parser.py

# == Build Plic Executables ==
CONFIGINT = "pyutilsdir" : "${abs_srcdir}",	"PLIC_VERSION" : "${VERSION}", "INTERN" : 1
CONFIGBIN = "pyutilsdir" : "${pyutilsdir}",	"PLIC_VERSION" : "${VERSION}"
${PLICINT}: plic.py Parser.py
	$(AM_V_GEN)
	$(Q) sed <$< >xgen-$(@F)			\
	  -e '1,1s|#!/usr/bin/env python\([0-9]\+\(\.[0-9]\+\)\?\)\?|#!${PYTHON}|' \
	  -e '1,24s|^ *#@PKGINSTALL_CONFIGVARS_IN24LINES@|  ${CONFIGINT}|'
	$(Q) chmod +x xgen-$(@F)
	$(Q) cp -p xgen-$(@F) $@
	$(Q) rm -f xgen-$(@F) xgen-$(@F).errors
${PLICV}: plic.py Parser.py
	$(AM_V_GEN)
	$(Q) sed <$< >xgen-$(@F)			\
	  -e '1,1s|#!/usr/bin/env python\([0-9]\+\(\.[0-9]\+\)\?\)\?|#!${PYTHON}|' \
	  -e '1,24s|^ *#@PKGINSTALL_CONFIGVARS_IN24LINES@|  ${CONFIGBIN}|'
	$(Q) chmod +x xgen-$(@F)
	$(Q) cp -p xgen-$(@F) $@
	$(Q) rm -f xgen-$(@F) xgen-$(@F).errors
EXTRA_DIST += plic.py

# == Plic Builtins ==
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

# == Plic Dependencies ==
pyutilsdir     = $(libdir)/$(strip ${PLICV})/pyutils
plicbackends   = PrettyDump.py TypeMap.py Rapicorn.py GType.py CxxStub.py
pyutils_PYTHON = Decls.py GenUtils.py Parser.py AuxData.py yapps2runtime.py ${plicbackends}

# == Basic Plic Test ==
check-plic-intern-list-formats:
	$(Q) ${PLICINT} --list-formats | grep -q PLIC ; eval "$$TSTDIAGNOSE"
check-local: check-plic-intern-list-formats

# == Post-Installation Test ==
test-plic-installation:
	$(Q) ${PLICV} --list-formats | fgrep -q CxxStub ; eval "$$TSTDIAGNOSE"
installcheck-local: test-plic-installation

# == 'plic' executable link ==
install-exec-hook:
	@echo "  LINK  " $(bindir)/$(PLICLINK)
	$(Q) cd $(DESTDIR)$(bindir)			\
	  && test -x ${PLICLINK}			\
	  || (rm -f ${PLICLINK} && $(LN_S) ${PLICV} ${PLICLINK} )
DESTDIRbinLINK = $(DESTDIR)$(bindir)/$(PLICLINK)
uninstall-hook:	# runs after ${PLICV} was uninstalled
	@    test ! -L $(DESTDIRbinLINK) -o -x $(DESTDIRbinLINK) || echo "  UNLINK" $(bindir)/$(PLICLINK)
	$(Q) test ! -L $(DESTDIRbinLINK) -o -x $(DESTDIRbinLINK) || rm -f $(DESTDIRbinLINK)

clean-local:
	rm -f *.pyc
