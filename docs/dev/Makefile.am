# Build and upload development documentation
include $(top_srcdir)/Makefile.decl
include Makefile.doxygen

REL_VERSION	= $(shell git describe --always --abbrev=0 HEAD 2>/dev/null)
DSC_VERSION	= $(shell git describe --always --match '[0-9]*' HEAD 2>/dev/null)
PRJ_VERSION	= $(shell sed -ne "/^[ \t]*VERSION[ \t]*=/ { s/^[^=]*=[ \t]*//; p; q }" < $(top_srcdir)/Makefile)
DOC_VERSION	= $(or $(DSC_VERSION), $(PRJ_VERSION))
git_stamp	= $(wildcard $(top_srcdir)/.git/$(shell git symbolic-ref -q HEAD) $(top_srcdir)/.git/packed-refs)
TARGET_HTML_DIRECTORY	= $(or $(DSC_VERSION), $(PRJ_VERSION))
DOXYGEN_DOC_FILES	= rapicorn-core.hh rapicorn.hh
DOXYGEN_DOC_DIRS	= rcore/ aidacc/ rope/ tools/ ui/ docs/tutorial/
DOXYGEN_EXCLUDES	= rope/cpy2rope.cc rope/cxx-client.[hc][hc] rcore/signalvariants.hh
DOXYGEN_CHANGELOG_TITLE	= "Rapicorn Development ChangeLog"
QPRINT		        = @printf '  %-7s%s\n'

all: build-stamp
build-stamp:
	$(Q) $(MAKE) --no-print-directory all-docs
	$(Q) touch $@
CLEANFILES = build-stamp

LINK_PROJECT_NAME = -e 's|<div id="projectname">Rapicorn|<div id="projectname"><a href="http://rapicorn.org">Rapicorn</a>|'

all-docs: $(git_stamp)
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory doxygen-html
	$(Q) sed $(LINK_PROJECT_NAME) -i doxygen-html/*.html
	$(QPRINT) "RSYNC" "HTML Documentation..."
	$(Q) mkdir -p html/ && rsync -aH --del doxygen-html/ html/$(TARGET_HTML_DIRECTORY)
	$(Q) rm -f html/latest && ln -s $(TARGET_HTML_DIRECTORY) html/latest
	$(QPRINT) "CLEAN" "Temporaries..."
	$(Q) rm -rf doxygen-html/

clean-local:
	rm -rf html/

# upload release versions directly, otherwise upload as latest/
upload: all
	$(Q) test "${REL_VERSION}" != "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html/${DOC_VERSION}"  "testbit:/srv/dev/html/rapicorn/" ; }
	$(Q) test "${REL_VERSION}" = "${DSC_VERSION}" || { set -x ; \
	  rsync -zaHP --del "html/${DOC_VERSION}/"  "testbit:/srv/dev/html/rapicorn/latest/" ; }
