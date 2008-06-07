# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

SUBDIRS = . tests

PLICV       = plic-@PLIC_VERSION@
PLICL       = plic

bin_PROGRAMS = ${PLICV}
# build parser
Parser.py: Parser.g
	yapps Parser.g xgen-$(@F) 2>&1 | tee xgen-$(@F).errors
	@! grep -q '.' xgen-$(@F).errors			# catch all yapps errors & warnings
	sed 's/^from yapps import runtime$$//' -i xgen-$(@F)	# parser.g has all imports
	cp -p xgen-$(@F) $@
	rm -f xgen-$(@F) xgen-$(@F).errors
EXTRA_DIST += Parser.g Parser.py
# build plic executable
plic-@PLIC_VERSION@: plic.py Parser.py
	cp $< xgen-$(@F)
	sed 's|^#@PLICSUBST_PREIMPORT@|sys.path.insert (0, "${pyutilsdir}")|' -i xgen-$(@F)
	sed 's|^"@PLICSUBST_VERSION@"|"${VERSION}"|' -i xgen-$(@F)
	chmod +x xgen-$(@F)
	cp -p xgen-$(@F) $@
	rm -f xgen-$(@F) xgen-$(@F).errors
EXTRA_DIST += plic.py

# install plic support module yapps2runtime.py
pyutils_PYTHON = yapps2runtime.py
pyutilsdir     = $(libdir)/rapicorn-@PLIC_VERSION@/pyutils
pyutils_PYTHON = Decls.py Parser.py yapps2runtime.py

# install/uninstall a "plic" link to "plic-PLIC_VERSION"
install-exec-hook:
	cd $(DESTDIR)$(bindir)			\
	&& test -x ${PLICL}			\
	|| $(LN_S) ${PLICV} ${PLICL}
uninstall-hook:	# runs after ${PLICV} was uninstalled
	cd $(DESTDIR)$(bindir)			\
	&& test ! -L ${PLICL} -o -x ${PLICL}	\
	|| rm -f ${PLICL}
