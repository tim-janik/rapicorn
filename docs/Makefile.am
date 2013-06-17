# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS = imports tutorial

QPRINT = @printf '  %-7s%s\n'

# Describe source tree version (release tag relative, e.g. yy.mm.z-nnn-gHEXHEX)
DSC_VERSION	= $(shell git describe --always --match '[0-9]*' HEAD 2>/dev/null)
# Release version, shows last tag reachable
REL_VERSION	= $(shell git describe --always --abbrev=0 HEAD 2>/dev/null)
# Project version as configured
PRJ_VERSION	= $(shell sed -ne "/^[ \t]*VERSION[ \t]*=/ { s/^[^=]*=[ \t]*//; p; q }" < $(top_srcdir)/Makefile)
# Detailed documentation version
DOC_VERSION = $(or $(DSC_VERSION), $(PRJ_VERSION))

# == Extra Tarball Files ==
EXTRA_DIST += ChangeLog-2009

# == Doxygen Install Rules ==
htmldocsbase  = ${docdir}
htmldocsdir   = ${htmldocsbase}/html
htmldocs_DATA = html/doc-version		# build dependency for doxygen docs
install-data-hook: @INGIT@ html/doc-version
	umask 022 && cp -RP html/ "$(DESTDIR)$(htmldocsbase)/"
uninstall-local:
	rm -Rf "$(DESTDIR)$(htmldocsbase)/html/"

# == Doxygen Build Rules ==
include Makefile.doxygen
git_index_file  = $(shell test ! -x $(top_srcdir)/.git/ || echo $(top_srcdir)/.git/index)
html/doc-version: @INGIT@ $(git_index_file)	# conditionally rebuild, depending on HEAD changes
	$(AM_V_GEN)
	$(Q) test -e $@ && test "`cat $@`" = "$(DOC_VERSION)" \
	|| $(MAKE) $(AM_MAKEFLAGS) --no-print-directory html-docs
	$(Q) rm -f $@ && echo "$(DOC_VERSION)" > $@
clean-html:
	rm -rf html/
clean-local: @INGIT@ clean-html doxygen-clean
EXTRA_DIST += html
#
LINK_PROJECT_NAME = -e 's|<div id="projectname">Rapicorn|<div id="projectname"><a href="http://rapicorn.org">Rapicorn</a>|'
HTMLMAN_FILES     = imports/Rapidrun.1.html
#
DOXYGEN_DOC_FILES	= rapicorn-core.hh rapicorn.hh docs/main.dox
DOXYGEN_DOC_DIRS	= rcore/ aidacc/ rope/ tools/ ui/ docs/tutorial/
DOXYGEN_EXCLUDES	= rope/cpy2rope.cc rope/cxx-client.[hc][hc] rcore/signalvariants.hh
DOXYGEN_CHANGELOG       = $(top_srcdir)/ChangeLog
DOXYGEN_CHANGELOG_TITLE	= "Rapicorn Development ChangeLog"
html-docs: doxygen-check
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-clean
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-srctree
	$(QPRINT) "GEN" "docs/docextract.dox"
	$(Q) cd doxygen-srctree && \
	  find . -type f -name \*[hcHC] -print | \
	  $(PYTHON) $(abs_srcdir)/docextract.py > docs/docextract.dox
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-html DOXYGEN_GRAPHICS=NO
	$(Q) sed $(LINK_PROJECT_NAME) -i doxygen-html/*.html
	$(QPRINT) "COPY" "Manual pages..."
	$(Q) cp $(HTMLMAN_FILES) doxygen-html/
	$(QPRINT) "RSYNC" "HTML Documentation..."
	$(Q) mkdir -p html/ && rsync -aH --del doxygen-html/ html/
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-clean
.PHONY: html-docs

# == Doxygen Upload ==
# Upload release versions directly, otherwise upload as latest/
upload: all
	$(Q) test "${REL_VERSION}" != "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html"  "testbit:/srv/dev/html/rapicorn/${DOC_VERSION}" ; }
	$(Q) test "${REL_VERSION}" = "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html/"  "testbit:/srv/dev/html/rapicorn/latest/" ; }
