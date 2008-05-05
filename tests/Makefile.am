# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= $(RAPICORN_DEP_CFLAGS) -DRAPICORN_LOG_DOMAIN='"$(basename $(@F))"'
DEFS            += $(GLIB_CFLAGS) # FIXME

XML_FILES   =
noinst_DATA =


xmllint-check:
	cd $(srcdir) && $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
