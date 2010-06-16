# Rapicorn
# Copyright (C) 2010 Tim Janik
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl


docdir        = ${datarootdir}/doc/rapicorn@RAPICORN_RELEASE@

tutorialdir   = ${docdir}/tutorial
tutorial_DATA = $(HTML_TARGETS) $(PY_TARGETS)
EXTRA_DIST   += $(tutorial_DATA)

HTML_TARGETS = html/pyhello.html
PY_TARGETS   = html/pyhello.py
CLEANFILES   += $(HTML_TARGETS) $(PY_TARGETS)

html/%.html: %.txt
	@mkdir -p html/
	doxer.py markup2html $< > $@

html/%.py: %.txt
	@mkdir -p html/
	sed -n '/^\t/{s/^\t//;p}' < $< > $@

clean-html:
	[ ! -d html ] || rm -r html/
clean-local: clean-html
