# Aida - Abstract Interface Definition Architecture
include $(top_srcdir)/Makefile.decl

SUBDIRS = . tests


bin_SCRIPTS         = ${AIDACCV}
noinst_SCRIPTS      = ${AIDACC_INTERN}
AIDACCLNK	    = aidacc			## user visible excutable name
AIDACCV	            = aidacc-@MAJOR@.@MINOR@	## installed executable name
AIDACC_INTERN	    = ./aidacc-intern		## internal version used during build time
AIDACC_VDEBUG	    = $(AIDACC_INTERN) $(if $(findstring 1, $(V)), --aida-debug)

CLEANFILES         += ${AIDACC_INTERN} ${AIDACCV}
YAPPS 		    = ${PYTHON} ${top_srcdir}/yapps2_deb/yapps2.py

# extra docs
EXTRA_DIST         += about-aida.dox xmanifold.py

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

GPRINT      = @printf '  %-9s%s\n'

# == Install aidacc Helper Modules ==
pyutilsdir      = $(libdir)/$(strip ${AIDACCV})/pyutils
pyutils_PYTHON  = Decls.py GenUtils.py Parser.py AuxData.py yapps2runtime.py ${backendfiles}
backendfiles    = PrettyDump.py TypeMap.py CxxStub.py ExtractDocs.py TmplFiles.py
noinst_SCRIPTS += $(backendfiles)

# == Templates & Copy-Paste Files ==
template_files = CxxStub-client.cc CxxStub-server.cc
EXTRA_DIST += $(template_files)
TmplFiles.py: Makefile.am $(template_files)
	$(AM_V_GEN)
	$(Q) (cd $(srcdir) && echo "### == $@ - Generated File == ###" \
	&& for file in $(template_files) ; \
	    do echo -n "$$file" | sed 's/[^a-zA-Z0-9_]/_/g' && echo ' = """' \
	    && sed 's/\\/\\\\/g; s/"""/\\"""/g' $$file && echo '"""' || exit $? ; done ) > xgen-$@
	$(Q) mv xgen-$@ $@
CLEANFILES += TmplFiles.py

# == Basic Aidacc Test ==
check-aidacc-intern-list-formats:
	$(Q) ${AIDACC_VDEBUG} -x TypeMap --list-formats | grep -q AidaTypeMap ; eval "$$TSTDIAGNOSE"
check-local: check-aidacc-intern-list-formats

# == Post-Installation Test ==
test-aidacc-installation:
	$(Q) PATH="$(bindir):$$PATH" ${AIDACCV} -x CxxStub -x ExtractDocs -x TypeMap --list-formats >$(TSTOUT) && \
	  fgrep -q CxxStub $(TSTOUT) && fgrep -q TypeMap $(TSTOUT) && fgrep -q ExtractDocs $(TSTOUT) ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f $(TSTOUT)
installcheck-local: test-aidacc-installation

# == 'aidacc' executable link ==
install-exec-hook: # runs after ${AIDACCV} was installed
	$(Q) cd $(DESTDIR)$(bindir) && \
	  $(call SYMLINK_LATEST, $(strip $(AIDACCLNK))-[0-9][0-9].[0-9][0-9], $(strip $(AIDACCLNK))) && \
	  test -x $(AIDACCLNK) -a -L $(AIDACCLNK)
uninstall-hook:	# runs after ${AIDACCV} was uninstalled
	$(Q) cd $(DESTDIR)$(bindir) && \
	  $(call SYMLINK_LATEST, $(strip $(AIDACCLNK))-[0-9][0-9].[0-9][0-9], $(strip $(AIDACCLNK))) && \
	  test -x $(AIDACCLNK) -o ! -e $(AIDACCLNK)

clean-local:
	rm -f *.pyc
