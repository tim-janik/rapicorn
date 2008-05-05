# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= $(RAPICORN_DEP_CFLAGS) -DRAPICORN_LOG_DOMAIN='"$(basename $(@F))"'
DEFS            += $(GLIB_CFLAGS) # FIXME

noinst_DATA =
XML_FILES   =
EXTRA_DIST += $(XML_FILES)

RAPIDRUN = $(top_builddir)/tools/rapidrun
IMGCHECK = $(top_builddir)/rapicorn-core/tests/imgcheck

# alignment test
alignment-test: $(srcdir)/alignment.xml
	rm -f xtmp-alignment.png
	$(RAPIDRUN) -x $(srcdir)/alignment.xml
	$(IMGCHECK) alignment.png xtmp-alignment.png
	rm -f xtmp-alignment.png
XML_FILES  += alignment.xml
EXTRA_DIST += alignment.png

check: alignment-test

xmllint-check:
	cd $(srcdir) && $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
