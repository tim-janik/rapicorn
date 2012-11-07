# Aida - Abstract Interface Definition Architecture
include $(top_srcdir)/Makefile.decl

SUBDIRS = . tests


bin_SCRIPTS         = ${AIDACCV}
noinst_SCRIPTS      = ${AIDACC_INTERN}
AIDACCLNK	    = aidacc			## user visible excutable name
AIDACCV	            = aidacc-@RAPICORN_RELEASE@	## installed executable name
AIDACC_INTERN	    = ./aidacc-intern		## internal version used during build time
CXXSTUB_PY 	    = $(srcdir)/CxxStub.py
CLEANFILES         += ${AIDACC_INTERN} ${AIDACCV}
YAPPS 		    = ${PYTHON} ${top_srcdir}/yapps2_deb/yapps2.py

# extra docs
EXTRA_DIST         += about-aida.dox

# == Build Parser ==
Parser.py: Parser.g
	${YAPPS} Parser.g xgen-$(@F) 2>&1 | tee xgen-$(@F).errors
	@! grep -q '.' xgen-$(@F).errors			# catch all yapps errors & warnings
	sed <xgen-$(@F) >$@			\
	    -e 's/^from yapps import runtime$$//'		# parser.g has all imports
	rm -f xgen-$(@F) xgen-$(@F).errors
EXTRA_DIST += Parser.g Parser.py
MAINTAINERCLEANFILES += Parser.py

# == Build Aida Executables ==
CONFIGINT = "pyutilsdir" : "${abs_srcdir}",	"AIDA_VERSION" : "${VERSION}",	"INTERN" : 1
CONFIGBIN = "pyutilsdir" : "${pyutilsdir}",	"AIDA_VERSION" : "${VERSION}"
${AIDACC_INTERN}: plic.py Parser.py
	$(AM_V_GEN)
	$(Q) sed <$< >xgen-$(@F)			\
	  -e '1,1s|#!/usr/bin/env python\([0-9]\+\(\.[0-9]\+\)\?\)\?|#!${PYTHON}|' \
	  -e '1,24s|^ *#@PKGINSTALL_CONFIGVARS_IN24LINES@|  ${CONFIGINT}|'
	$(Q) chmod +x xgen-$(@F)
	$(Q) cp -p xgen-$(@F) $@
	$(Q) rm -f xgen-$(@F) xgen-$(@F).errors
${AIDACCV}: plic.py Parser.py
	$(AM_V_GEN)
	$(Q) sed <$< >xgen-$(@F)			\
	  -e '1,1s|#!/usr/bin/env python\([0-9]\+\(\.[0-9]\+\)\?\)\?|#!${PYTHON}|' \
	  -e '1,24s|^ *#@PKGINSTALL_CONFIGVARS_IN24LINES@|  ${CONFIGBIN}|'
	$(Q) chmod +x xgen-$(@F)
	$(Q) cp -p xgen-$(@F) $@
	$(Q) rm -f xgen-$(@F) xgen-$(@F).errors
EXTRA_DIST += plic.py

# == Aida Builtins ==
builtins.cc: $(srcdir)/builtins.idl ${AIDACC_INTERN} plic.py
	$(AM_V_GEN)
	$(Q) ${AIDACC_INTERN} --system-typedefs $< -G TypeMap -o $(@F).typ
	$(Q) ${AIDACC_INTERN} --cc-intern-file $(@F).typ > xtmp-$(@F)
	$(Q) cp -p xtmp-$(@F) $@
	$(Q) rm -f $(@F).typ xtmp-$(@F)
noinst_DATA = builtins.cc
EXTRA_DIST += builtins.cc builtins.idl
CLEANFILES += builtins.cc

# == Aida Dependencies ==
pyutilsdir     = $(libdir)/$(strip ${AIDACCV})/pyutils
aidaccbackends = PrettyDump.py TypeMap.py CxxStub.py
pyutils_PYTHON = Decls.py GenUtils.py Parser.py AuxData.py yapps2runtime.py ${aidaccbackends}

# == Basic Aidacc Test ==
check-aidacc-intern-list-formats:
	$(Q) ${AIDACC_INTERN} --list-formats | grep -q AidaTypeMap ; eval "$$TSTDIAGNOSE"
check-local: check-aidacc-intern-list-formats

# == Post-Installation Test ==
test-aidacc-installation:
	$(Q) PATH="$(bindir):$$PATH" ${AIDACCV} --list-formats | fgrep -q CxxStub ; eval "$$TSTDIAGNOSE"
installcheck-local: test-aidacc-installation

# == 'aidacc' executable link ==
install-exec-hook:
	@echo "  LINK  " $(bindir)/$(AIDACCLNK)
	$(Q) cd $(DESTDIR)$(bindir)			\
	  && test -x ${AIDACCLNK}			\
	  || (rm -f ${AIDACCLNK} && $(LN_S) ${AIDACCV} ${AIDACCLNK} )
DESTDIRbinLINK = $(DESTDIR)$(bindir)/$(AIDACCLNK)
uninstall-hook:	# runs after ${AIDACCV} was uninstalled
	@    test ! -L $(DESTDIRbinLINK) -o -x $(DESTDIRbinLINK) || echo "  UNLINK" $(bindir)/$(AIDACCLNK)
	$(Q) test ! -L $(DESTDIRbinLINK) -o -x $(DESTDIRbinLINK) || rm -f $(DESTDIRbinLINK)

clean-local:
	rm -f *.pyc
