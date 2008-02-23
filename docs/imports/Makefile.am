# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

docdir = ${datarootdir}/doc/${PACKAGE}${RAPICORN_RELEASE_NUMBER}

tutorialdir   = $(docdir)/tutorial
tutorial_DATA = doxer.css Hello-World.html Rapicorn-Tutorial.html
EXTRA_DIST   += $(tutorial_DATA)

CHROME_CONF = --no-cache --header "X-Script-Chrome: chrome=off style=doxer.css"
WGET_MIRROR = wget -E -k -r -np -nd -nc
TMPDIR=tmphtml
import-docs:
	mkdir -p $(TMPDIR)
	rm -rf $(TMPDIR)/*
	$(DOXRULE)  cd $(TMPDIR) && doxer.py copy-css
	cd $(TMPDIR) && $(WGET_MIRROR) $(CHROME_CONF) 'http://testbit.eu/book/Rapicorn-Tutorial'
	mv $(TMPDIR)/*.html $(TMPDIR)/*.css .
	rm -rf $(TMPDIR)/
	touch stamp-import-docs
.PHONY: import-docs

dist-hook:
	find stamp-import-docs -mtime -4 | fgrep -q stamp \
	|| { echo "docs/imports/: Last import is outdated, please run: make import-docs" >&2 ; exit 1 ; }
