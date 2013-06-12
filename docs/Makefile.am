# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl
include Makefile.doxygen

SUBDIRS = imports tutorial

REL_VERSION	= $(shell git describe --always --abbrev=0 HEAD 2>/dev/null)
DSC_VERSION	= $(shell git describe --always --match '[0-9]*' HEAD 2>/dev/null)
PRJ_VERSION	= $(shell sed -ne "/^[ \t]*VERSION[ \t]*=/ { s/^[^=]*=[ \t]*//; p; q }" < $(top_srcdir)/Makefile)
DOC_VERSION	= $(or $(DSC_VERSION), $(PRJ_VERSION))
TARGET_HTML_DIRECTORY	= $(or $(DSC_VERSION), $(PRJ_VERSION))
DOXYGEN_DOC_FILES	= rapicorn-core.hh rapicorn.hh
DOXYGEN_DOC_DIRS	= rcore/ aidacc/ rope/ tools/ ui/ docs/tutorial/
DOXYGEN_EXCLUDES	= rope/cpy2rope.cc rope/cxx-client.[hc][hc] rcore/signalvariants.hh
DOXYGEN_CHANGELOG_TITLE	= "Rapicorn Development ChangeLog"
QPRINT		        = @printf '  %-7s%s\n'

# fetch HEAD commit hash to figure if docs need to be rebuild
git_head_commit = $(shell test ! -x $(top_srcdir)/.git/ || git rev-parse HEAD)
git_index_file  = $(shell test ! -x $(top_srcdir)/.git/ || echo $(top_srcdir)/.git/index)

all: stamp-docu
stamp-docu: $(git_index_file)	# conditionally rebuild, depending on HEAD changes
	$(AM_V_GEN)
	$(Q) test -e html/ && test -e $@ && test "`cat $@`" = "$(git_head_commit)" \
	|| $(MAKE) $(AM_MAKEFLAGS) --no-print-directory all-docs
	$(Q) rm -f $@ && echo "$(git_head_commit)" > $@
CLEANFILES += stamp-docu

LINK_PROJECT_NAME = -e 's|<div id="projectname">Rapicorn|<div id="projectname"><a href="http://rapicorn.org">Rapicorn</a>|'

CHECK_DOXYGEN = { command -v doxygen >/dev/null && { echo "1.7.6.1" ; doxygen --version ; } | sort -VC ; }
CHECK_DOT     = { command -v dot >/dev/null && { echo "2.26.0" ; dot -V 2>&1 | sed 's/[^0-9]*//' ; } | sort -VC ; }

all-docs:
	$(Q) ${CHECK_DOXYGEN} || { echo "$@: failed to detect recent version: doxygen"; exit 1; }
	$(Q) ${CHECK_DOT}     || { echo "$@: failed to detect recent version: dot"; exit 1; }
	$(Q) rm -rf doxygen-srctree/
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-srctree
	$(QPRINT) "GEN" "docs/docextract.dox"
	$(Q) cd doxygen-srctree && \
	  find . -type f -name \*[hcHC] -print | \
	  $(PYTHON) $(abs_srcdir)/docextract.py > docs/docextract.dox
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-html
	$(Q) sed $(LINK_PROJECT_NAME) -i doxygen-html/*.html
	$(QPRINT) "RSYNC" "HTML Documentation..."
	$(Q) mkdir -p html/ && rsync -aH --del doxygen-html/ html/$(TARGET_HTML_DIRECTORY)
	$(Q) rm -f html/latest && ln -s $(TARGET_HTML_DIRECTORY) html/latest
	$(QPRINT) "CLEAN" "Temporaries..."
	$(Q) rm -rf doxygen-html/
.PHONY: all-docs

clean-local:
	rm -rf html/

# upload release versions directly, otherwise upload as latest/
upload: all
	$(Q) test "${REL_VERSION}" != "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html/${DOC_VERSION}"  "testbit:/srv/dev/html/rapicorn/" ; }
	$(Q) test "${REL_VERSION}" = "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html/${DOC_VERSION}/"  "testbit:/srv/dev/html/rapicorn/latest/" ; }


# == Extra Tarball Files ==
EXTRA_DIST += ChangeLog-2009
