# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS = imports tutorial

QPRINT = @printf '  %-7s%s\n'

# Describe source tree version (release tag relative, e.g. yy.mm.z-nnn-gHEXHEX)
DSC_VERSION = $(shell git describe --always --match '[0-9]*' HEAD 2>/dev/null)
# Release version, shows last tag reachable
REL_VERSION = $(shell git describe --always --abbrev=0 HEAD 2>/dev/null)
# Project version as configured
PRJ_VERSION = $(shell sed -ne "/^[ \t]*VERSION[ \t]*=/ { s/^[^=]*=[ \t]*//; p; q }" < $(top_srcdir)/Makefile)
# Detailed documentation version
DOC_VERSION = $(or $(DSC_VERSION), $(PRJ_VERSION))

# == Extra Tarball Files ==
EXTRA_DIST += ChangeLog-2009 references.bib

# == Doxygen Install Rules ==
htmldocsbase  = ${docdir}
htmldocsdir   = ${htmldocsbase}/html
htmldocs_DATA = html/doc-version		# build dependency for doxygen docs
htmlparent    = $(if $(findstring \#, ..\@INGIT@..), $(srcdir), $(builddir))
install-data-hook: @INGIT@ $(htmldocs_DATA)
	umask 022 && cp -RP $(htmlparent)/html/ "$(DESTDIR)$(htmldocsbase)/"
	chmod -R u+w "$(DESTDIR)$(htmldocsbase)/"
# u+w is needed for uninstall (distcheck enforces read-only srcdir contents)
uninstall-local:
	rm -Rf "$(DESTDIR)$(htmldocsbase)/html/"

# == html/ build rules ==
include Makefile.doxygen
git_index_file = $(shell test ! -x $(top_srcdir)/.git/ || echo $(top_srcdir)/.git/index)
html/doc-version: @INGIT@ $(git_index_file)	# conditionally rebuild, depending on HEAD changes
	$(AM_V_GEN)
	$(Q) test -e $@ && test "`cat $@`" = "$(DOC_VERSION)" \
	|| $(MAKE) $(AM_MAKEFLAGS) --no-print-directory html-docs
	$(Q) rm -f $@ && echo "$(DOC_VERSION)" > $@
clean-html-docs:
	rm -rf html/
clean-local: @INGIT@ clean-html-docs doxygen-clean
EXTRA_DIST += html
html-docs: doxygen-check
	$(Q) rm -rf html/
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory build-docs DOXYGEN_GRAPHICS=YES DOC_TARGET=html
	$(Q) echo "$(DOC_VERSION)" > html/doc-version
.PHONY: html-docs

# == Doxygen Build Rules ==
HTMLMAN_FILES     	= imports/rapidrun.1.html
DOXYGEN_DOC_FILES	= rapicorn-core.hh rapicorn.hh docs/main.dox
DOXYGEN_DOC_DIRS	= rcore/ aidacc/ rope/ tools/ ui/ docs/tutorial/
DOXYGEN_EXCLUDES	= rope/cpy2rope.cc rope/cxx-client.[hc][hc] rcore/signalvariants.hh ui/sinfeximpl.hh
DOXYGEN_CHANGELOG       = $(top_srcdir)/ChangeLog
DOXYGEN_CHANGELOG_TITLE	= "Rapicorn Development ChangeLog"
DOXYGEN_NEWS            = $(top_srcdir)/NEWS
DOXYGEN_PROJECT_LINK    = http://rapicorn.org
DOXYGEN_TAGFILES        = imports/tagfile-susv4.xml=http://pubs.opengroup.org/onlinepubs/9699919799/
DOXYGEN_STYLESHEET	= extrastyles.css
SED_DELETE_ATFILE_LINE1	= '1s,/\*\*\s*@file.*\*/\s*,,'
build-docs: doxygen-check
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-clean
	$(QPRINT) "GEN" "Git ChangeLog"
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-srctree
	$(Q) cp $(srcdir)/references.bib doxygen-srctree/
	$(QPRINT) "MERGE" "ui/interfaces.dox"
	$(Q) cd doxygen-srctree/ui && \
	  sed $(SED_DELETE_ATFILE_LINE1) < interfaces.dox >> interfaces.idl && \
	  rm interfaces.dox
	$(QPRINT) "GEN" "docs/docextract.dox"
	$(Q) cd doxygen-srctree && \
	  find . -type f -name \*[hcHC] -print | \
	  $(PYTHON) $(abs_srcdir)/docextract.py > docs/docextract.dox
	$(QPRINT) "HIDE" "internal enum definitions"
	$(Q) cd doxygen-srctree && \
	  find . -type f -name \*[cC] -exec sed 's/^\([^"]*\)\benum\b/\1Enum/' -i {} \;
	$(Q) bunzip2 -fk imports/tagfile-susv4.xml.bz2
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-html
	$(Q) rm -f imports/tagfile-susv4.xml
	$(QPRINT) "GEN" "HTML manual pages..."
	$(Q) cp $(HTMLMAN_FILES) doxygen-html/
	$(QPRINT) "MOVE" "HTML Documentation..."
	$(Q) ! test -e $(DOC_TARGET) || { echo "$@: target directory exists already: $(DOC_TARGET)"; exit 1; }
	$(Q) mv doxygen-html $(DOC_TARGET)
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-clean
.PHONY: build-docs

# == Upload build rules ==
upload-docs: doxygen-check $(git_index_file)
	$(AM_V_GEN)
	$(Q) test -e $@ && test "`cat upload-docs/doc-version`" = "$(DOC_VERSION)" || (true \
	&& rm -rf upload-docs/ \
	&& $(MAKE) $(AM_MAKEFLAGS) --no-print-directory build-docs DOXYGEN_GRAPHICS=YES DOC_TARGET=upload-docs )
	$(Q) echo "$(DOC_VERSION)" > upload-docs/doc-version
clean-upload-docs:
	rm -rf upload-docs/
clean-local: clean-upload-docs
# Upload release versions as $DOC_VERSION, otherwise upload as latest/
upload: upload-docs
	$(AM_V_GEN)
	$(Q) ${CHECK_RSYNC} || { echo "$@: failed to detect recent version: rsync"; exit 1; }
	$(Q) test "${REL_VERSION}" != "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "upload-docs"   "testbit:pub/docs/rapicorn/${DOC_VERSION}" ; }
	$(Q) test "${REL_VERSION}"  = "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "upload-docs/"  "testbit:pub/docs/rapicorn/latest/" ; }
.PHONY: upload upload-docs
CHECK_RSYNC   = { command -v rsync >/dev/null && { echo "3.0.0" ; rsync --version 2>&1 | sed 's/[^0-9]*// ; 1q' ; } | sort -VC ; }
