# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_GUI_CFLAGS)

# == Build Classes ==
noinst_PROGRAMS	 = $(X11TESTS) $(NOINSTPRGS)
noinst_DATA	 =
LDADDS		 = $(top_builddir)/ui/librapicorn-@MAJOR@.la
XML_FILES	 =
EXTRA_DIST	+= $(XML_FILES)
NOINSTPRGS =

# == Programs ==
NOINSTPRGS        += imgcheck
imgcheck_SOURCES   = imgcheck.cc
imgcheck_LDADD     = $(LDADDS)

# environment
export VPATH   # needed by test programs to find builddir relative input files

# == Test Programs ==
X11TESTS            += servertests
servertests_SOURCES  = servertests.cc cmdtest.cc properties.cc region.cc server.cc labelmarkup.cc \
		       widgets.cc primitives.cc testwidgets.cc sinfextest.cc stores.cc testselector.cc
servertests_LDADD    = $(LDADDS) $(READLINELIBS)
XML_FILES    	    += factory.xml testwidgets.xml
EXTRA_DIST          += servertests.hh $(XML_FILES)
X11TESTS            += clienttests
clienttests_SOURCES  = clienttests.cc pixmaps.cc
clienttests_LDADD    = $(LDADDS)
EXTRA_DIST          += testpixs.c testpixs.png

SINFEX_SHELL=./servertests --shell
check: sinfex-expressions
sinfex-expressions: $(srcdir)/expressions.ref ./servertests
	$(Q) egrep -v '^[[:space:]]*(#|$$)' < $(srcdir)/expressions.txt > $(TSTTMP)
	$(Q) $(X11_ENV) COLUMNS=9999 \
		$(SINFEX_SHELL) < $(TSTTMP) > $(TSTOUT) \
	; eval "$$TSTDIAGNOSE_LOGTEST"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/expressions.ref'" "'$(TSTOUT)'"
	$(Q) rm -f $(TSTTMP)
EXTRA_DIST += expressions.txt expressions.ref

xmllint-check:
	cd $(srcdir) && ! $(XMLLINT) --noout $(XML_FILES) 2>&1 | grep '.'
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES  += stamp-autochecks
