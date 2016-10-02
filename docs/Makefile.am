# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS = examples imports tutorial

QPRINT = @printf '  %-9s%s\n'

# Describe documentation version by real version number and git relative
DSC_VERSION = $(shell git describe --always --match '[0-9]*' HEAD 2>/dev/null || echo tarball)
DOC_VERSION = $(VERSION): $(DSC_VERSION)

# == Extra Tarball Files ==
EXTRA_DIST += ChangeLog-2009 references.bib doxygen_footer.html

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
html/doc-version: @INGIT@ $(if $(NODOC),, $(git_index_file)) # conditionally rebuild, depending on HEAD changes
	$(AM_V_GEN)
	$(Q) test -e $@ && test "`cat $@`" = "$(DOC_VERSION)" \
	|| $(MAKE) $(AM_MAKEFLAGS) --no-print-directory html-docs
	$(Q) test -e $@ # created by 'html-docs'

# == Doxygen Build Rules ==
HTMLMAN_FILES     	= mans/rapidrun.1.html mans/rapidres.1.html
DOXYGEN_DOC_FILES	= rapicorn-core.hh rapicorn.hh docs/main.dox docs/apps.dox docs/contribute.dox docs/configuration.dox
DOXYGEN_DOC_DIRS	= rcore/ aidacc/ tools/ ui/ docs/tutorial/
DOXYGEN_EXCLUDES	= rcore/signalvariants.hh ui/sinfeximpl.hh ui/regionimpl.c \
			  aidacc/CxxStub.py
DOXYGEN_CHANGELOG       = $(builddir)/devel-changes
DOXYGEN_CHANGELOG_TITLE	= "Development Changes"
DOXYGEN_NEWS            = $(top_srcdir)/NEWS
DOXYGEN_LAYOUT_DIFF     = $(srcdir)/layout.diff
DOXYGEN_PROJECT_LINK    = http://rapicorn.org
DOXYGEN_TAGFILES        = $(strip \
	imports/tagfile-susv4.xml=http://pubs.opengroup.org/onlinepubs/9699919799/	\
	imports/cppreference-doxygen-web.tag.xml=http://en.cppreference.com/w/		\
)
DOXYGEN_STYLESHEET	= extrastyles.css
SED_DELETE_ATFILE_LINE1	= '1s,/\*\*\s*@file.*\*/\s*,,'
html-docs: doxygen-check $(top_srcdir)/ChangeLog
	$(AM_V_GEN)
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-clean
	$(QPRINT) "GEN" "Documentation Sources"
	$(Q) sed -e '/# d1941418d18/Q' < $(top_srcdir)/ChangeLog > $(DOXYGEN_CHANGELOG) # cut log length to stay web-browsable
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-srctree
	$(Q) rm -f $(DOXYGEN_CHANGELOG)
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
	$(Q) bunzip2 -fk imports/tagfile-susv4.xml.bz2 imports/cppreference-doxygen-web.tag.xml.bz2
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-html
	$(Q) rm -f imports/tagfile-susv4.xml imports/cppreference-doxygen-web.tag.xml
	$(QPRINT) "GEN" "CSS for Rapicorn..."
	$(Q) test ! -e doxygen-html/navtree.css \
	  || sed -r 's/\bfont\b[^:;]*:[^;]*;//' -i doxygen-html/navtree.css
	$(Q) sed -r 's/\bfont(-size)?:[^;]*;//' -i doxygen-html/doxygen.css
	$(QPRINT) "GEN" "HTML manual pages..."
	$(Q) cp $(HTMLMAN_FILES) doxygen-html/
	$(QPRINT) "MOVE" "HTML Documentation..."
	$(Q) rm -rf html/
	$(Q) mv doxygen-html html/
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-clean
	$(Q) rm -f html/doc-version && echo "$(DOC_VERSION)" > html/doc-version
EXTRA_DIST += html
.PHONY: html-docs
clean-html:
	rm -rf html/
clean-local: doxygen-clean @INGIT@ clean-html

# == Coverage Build Rules ==
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
	$(Q) test ! -e "$(git_index_file)" -o "$(top_builddir)/$(DIST_TARBALL)" -nt "$(git_index_file)" || \
	  { echo "ERROR: needs rebuild: $(top_builddir)/$(DIST_TARBALL)"; false ; }
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
	    -e 's/<title>[^<]*<\/title>/<title>Rapicorn Code Coverage Report<\/title>/i' \
	    -e 's/lcov[ -]*\(code \)\?coverage.report/<a href="http:\/\/rapicorn.org">Rapicorn<\/a> Code Coverage Report/i' '{}' \;
	$(QPRINT) "GEN" "coverage/tarball-version"
	$(Q) rm -f coverage/tarball-version && echo "$(DIST_TARBALL) ($(DOC_VERSION))" > coverage/tarball-version
	$(QPRINT) "DONE" "Use: www-browser `pwd`/coverage/index.html"
clean-coverage:
	rm -rf coverage/ coverage-tree/
clean-local: clean-coverage

# == Upload Rule ==
CHECK_RSYNC = { command -v rsync >/dev/null && { echo "3.0.0" ; rsync --version 2>&1 | sed 's/[^0-9]*// ; 1q' ; } | sort -VC ; }
# Upload stable versions as $MAJOR.$MINOR, otherwise upload as latest/
upload-docs: html/doc-version
	$(AM_V_GEN)
	$(Q) ${CHECK_RSYNC} || { echo "$@: failed to detect recent version: rsync"; exit 1; }
	$(Q) docdir=latest && test "@ENABLE_DEVEL_MODE_TRUE@" != "#" || \
	  docdir="@MAJOR@.@MINOR@" && \
	  set -x && rsync -zaHP --del "html/" "testbit:pub/docs/rapicorn/$$docdir"

upload-coverage: coverage
	$(AM_V_GEN)
	$(Q) ${CHECK_RSYNC} || { echo "$@: failed to detect recent version: rsync"; exit 1; }
	$(Q) { set -x ; rsync -zaHP --del "coverage/"  "testbit:pub/docs/rapicorn/coverage/" ; }
upload: upload-coverage upload-docs
.PHONY: upload upload-docs upload-coverage
