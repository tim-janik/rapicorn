# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS = imports tutorial

QPRINT = @printf '  %-9s%s\n'

# Describe source tree version (release tag relative, e.g. yy.mm.z-nnn-gHEXHEX)
DSC_VERSION = $(shell git describe --always --match '[0-9]*' HEAD 2>/dev/null)
# Release version, shows last tag reachable
REL_VERSION = $(shell git describe --always --abbrev=0 HEAD 2>/dev/null)
# Detailed documentation version, falls back to package version for tarballs
DOC_VERSION = $(or $(DSC_VERSION), $(VERSION))

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
clean-html-docs:
	rm -rf html/
clean-local: @INGIT@ clean-html-docs doxygen-clean
EXTRA_DIST += html
html-docs: doxygen-check
	$(AM_V_GEN)
	$(Q) rm -rf html/
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory build-docs DOXYGEN_GRAPHICS=YES DOC_TARGET=html
	$(Q) rm -f html/doc-version && echo "$(DOC_VERSION)" > html/doc-version
.PHONY: html-docs

# == Doxygen Build Rules ==
HTMLMAN_FILES     	= imports/rapidrun.1.html
DOXYGEN_DOC_FILES	= rapicorn-core.hh rapicorn.hh docs/main.dox
DOXYGEN_DOC_DIRS	= rcore/ aidacc/ rope/ tools/ ui/ docs/tutorial/
DOXYGEN_EXCLUDES	= rope/cpy2rope.cc rope/cxx-client.[hc][hc] rcore/signalvariants.hh ui/sinfeximpl.hh ui/regionimpl.c \
			  aidacc/CxxStub.py
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
.PHONY: build-docs errors
errors: html-docs
	@sed 's,/docs/doxygen-srctree/,/,g' error.log

# == Coverage Rules ==
DIST_TARBALL = $(PACKAGE)-$(VERSION).tar.bz2
coverage-tree/buildid: @INGIT@ $(git_index_file)	# conditionally rebuild, depending on HEAD changes
	$(AM_V_GEN)
	$(Q) test -e $@ && test "`cat $@`" = "$(DOC_VERSION)" \
	|| { $(MAKE) $(AM_MAKEFLAGS) --no-print-directory coverage-tree && \
	     rm -f $@ && echo "$(DOC_VERSION)" > $@ ; }
coverage-tree:
	$(AM_V_GEN)
	$(QPRINT) "CLEAN" "coverage-tree/"
	$(Q) rm -rf coverage-tree/
	$(QPRINT) "FIND" "dist tarball: $(DIST_TARBALL)"
	$(Q) test -e "$(top_builddir)/$(DIST_TARBALL)" || { echo "ERROR: missing $(top_builddir)/$(DIST_TARBALL)"; false ; }
	$(QPRINT) "BUILD" "coverage-tree/ from tarball"
	$(Q) $(srcdir)/mkcoverage.sh --tree-only "$(top_builddir)/$(DIST_TARBALL)"
.PHONY: coverage-tree # 'make coverage-tree' forces rebuilds, needed by coverage-tree/buildid
coverage: coverage-tree/buildid
	$(AM_V_GEN)
	$(QPRINT) "CLEAN" "coverage/"
	$(Q) rm -rf coverage/
	$(QPRINT) "BUILD" "coverage/ from coverage-tree/"
	$(Q) $(srcdir)/mkcoverage.sh "$(top_builddir)/$(DIST_TARBALL)"
	$(QPRINT) "POLISH" "clean up coverage report titles"
	$(Q) find coverage/ -name '*.html' -exec sed -i \
	    -e 's/lcov[ -]*\(code \)\?coverage.report/Rapicorn Code Coverage Report/i' \
	    -e 's/<title>[^<]*<\/title>/<title>Rapicorn Code Coverage Report<\/title>/i' '{}' \;
	$(Q) rm -f coverage/tarball-version && echo "$(DIST_TARBALL) ($(DOC_VERSION))" > coverage/tarball-version
	$(QPRINT) "DONE" "Use: www-browser `pwd`/coverage/index.html"
clean-coverage:
	rm -rf coverage/ coverage-tree/
clean-local: clean-coverage

# == Upload build rules ==
CHECK_RSYNC = { command -v rsync >/dev/null && { echo "3.0.0" ; rsync --version 2>&1 | sed 's/[^0-9]*// ; 1q' ; } | sort -VC ; }
website-docs: doxygen-check $(git_index_file)
	$(AM_V_GEN)
	$(Q) test -e $@ && test "`cat website-docs/doc-version`" = "$(DOC_VERSION)" || (true \
	&& rm -rf website-docs/ \
	&& $(MAKE) $(AM_MAKEFLAGS) --no-print-directory build-docs DOXYGEN_GRAPHICS=YES DOC_TARGET=website-docs )
	$(Q) echo "$(DOC_VERSION)" > website-docs/doc-version
clean-website-docs:
	rm -rf website-docs/
clean-local: clean-website-docs
# Upload release versions as $DOC_VERSION, otherwise upload as latest/
upload: website-docs coverage
	$(AM_V_GEN)
	$(Q) ${CHECK_RSYNC} || { echo "$@: failed to detect recent version: rsync"; exit 1; }
	$(Q) test "${REL_VERSION}" != "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "website-docs"   "testbit:pub/docs/rapicorn/${DOC_VERSION}" ; }
	$(Q) test "${REL_VERSION}"  = "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "website-docs/"  "testbit:pub/docs/rapicorn/latest/" ; }
	$(Q) { set -x ; rsync -zaHP --del "coverage/"  "testbit:pub/docs/rapicorn/coverage/" ; }
.PHONY: upload
