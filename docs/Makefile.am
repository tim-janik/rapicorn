# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS = imports tutorial

# == Doxygen Build Rules ==
include Makefile.doxygen
htmldocsbase  = ${docdir}
htmldocsdir   = ${htmldocsbase}/html
htmldocs_DATA = html/stamp-id		# build dependency for doxygen docs
install-data-hook:
	cp -RP html/ "$(DESTDIR)$(htmldocsbase)/"
uninstall-local:
	rm -Rf "$(DESTDIR)$(htmldocsbase)/html/"

DOXYGEN_DOC_FILES	= rapicorn-core.hh rapicorn.hh
DOXYGEN_DOC_DIRS	= rcore/ aidacc/ rope/ tools/ ui/ docs/tutorial/
DOXYGEN_EXCLUDES	= rope/cpy2rope.cc rope/cxx-client.[hc][hc] rcore/signalvariants.hh
DOXYGEN_CHANGELOG_TITLE	= "Rapicorn Development ChangeLog"
QPRINT		        = @printf '  %-7s%s\n'

# fetch HEAD commit hash to figure if docs need to be rebuild
git_head_commit = $(shell test ! -x $(top_srcdir)/.git/ || git rev-parse HEAD)
git_index_file  = $(shell test ! -x $(top_srcdir)/.git/ || echo $(top_srcdir)/.git/index)

html/stamp-id: $(git_index_file)	# conditionally rebuild, depending on HEAD changes
	$(AM_V_GEN)
	$(Q) test -e html/ && test -e $@ && test "`cat $@`" = "$(git_head_commit)" \
	|| $(MAKE) $(AM_MAKEFLAGS) --no-print-directory all-docs
	$(Q) rm -f $@ && echo "$(git_head_commit)" > $@
clean-local:
	rm -rf html/

LINK_PROJECT_NAME = -e 's|<div id="projectname">Rapicorn|<div id="projectname"><a href="http://rapicorn.org">Rapicorn</a>|'

CHECK_DOXYGEN = { command -v doxygen >/dev/null && { echo "1.7.6.1" ; doxygen --version ; } | sort -VC ; }
CHECK_DOT     = { command -v dot >/dev/null && { echo "2.26.0" ; dot -V 2>&1 | sed 's/[^0-9]*//' ; } | sort -VC ; }
CHECK_RSYNC   = { command -v rsync >/dev/null && { echo "3.0.0" ; rsync --version 2>&1 | sed 's/[^0-9]*// ; 1q' ; } | sort -VC ; }

all-docs:
	$(Q) ${CHECK_DOXYGEN} || { echo "$@: failed to detect recent version: doxygen"; exit 1; }
	$(Q) ${CHECK_DOT}     || { echo "$@: failed to detect recent version: dot"; exit 1; }
	$(Q) ${CHECK_RSYNC}   || { echo "$@: failed to detect recent version: rsync"; exit 1; }
	$(Q) rm -rf doxygen-srctree/
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-srctree
	$(QPRINT) "GEN" "docs/docextract.dox"
	$(Q) cd doxygen-srctree && \
	  find . -type f -name \*[hcHC] -print | \
	  $(PYTHON) $(abs_srcdir)/docextract.py > docs/docextract.dox
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-html
	$(Q) sed $(LINK_PROJECT_NAME) -i doxygen-html/*.html
	$(QPRINT) "RSYNC" "HTML Documentation..."
	$(Q) mkdir -p html/ && rsync -aH --del doxygen-html/ html/
	$(QPRINT) "CLEAN" "Temporaries..."
	$(Q) rm -rf doxygen-html/
.PHONY: all-docs

# upload release versions directly, otherwise upload as latest/
REL_VERSION	= $(shell git describe --always --abbrev=0 HEAD 2>/dev/null)
DSC_VERSION	= $(shell git describe --always --match '[0-9]*' HEAD 2>/dev/null)
PRJ_VERSION	= $(shell sed -ne "/^[ \t]*VERSION[ \t]*=/ { s/^[^=]*=[ \t]*//; p; q }" < $(top_srcdir)/Makefile)
DOC_VERSION	= $(or $(DSC_VERSION), $(PRJ_VERSION))
upload: all
	$(Q) test "${REL_VERSION}" != "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html"  "testbit:/srv/dev/html/rapicorn/${DOC_VERSION}" ; }
	$(Q) test "${REL_VERSION}" = "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html/"  "testbit:/srv/dev/html/rapicorn/latest/" ; }


# == Extra Tarball Files ==
EXTRA_DIST += ChangeLog-2009
