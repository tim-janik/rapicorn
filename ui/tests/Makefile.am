# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) $(RAPICORN_GUI_CFLAGS)
DEFS	 += -DRAPICORN_CONVENIENCE -D__SOURCE_FILE__='"$(patsubst $(realpath $(abs_top_srcdir))/%,%, $(realpath $<))"'

# test programs
noinst_PROGRAMS	 = $(TEST_PROGS) $(LOGTEST_PROGS)
noinst_DATA	 =
LDADDS		 = $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la
XML_FILES	 =
EXTRA_DIST	+= $(XML_FILES)

# programs
TESTS_ENVIRONMENT  = RAPICORN_VPATH=$(VPATH) # also search srcdir for XML files

# test programs
LOGTEST_PROGS       += servertests
TEST_PROGS          += servertests
servertests_SOURCES  = servertests.cc walker.cc cmdtest.cc properties.cc region.cc server.cc labelmarkup.cc factory.cc items.cc
servertests_LDADD    = $(LDADDS)
XML_FILES    	    += factory.xml

#TEST_PROGS        += sinfextest
sinfextest_SOURCES = sinfextest.cc
sinfextest_LDADD   = $(LDADDS) $(READLINELIBS)
#TEST_PROGS        += testitems
testitems_SOURCES  = testitems.cc
testitems_LDADD    = $(LDADDS)
XML_FILES         += testitems.xml
#TEST_PROGS        += primitives
primitives_SOURCES = primitives.cc
primitives_LDADD   = $(LDADDS)

SINFEX=./sinfextest --shell
check: #sinfex-expressions
sinfex-expressions: $(srcdir)/expressions.ref
	$(Q) egrep -v '^[[:space:]]*(#|$$)' < $(srcdir)/expressions.txt > $(TSTTMP) \
	&& $(X11TEST_SERVER) && \
	  COLUMNS=9999 $(SINFEX) < $(TSTTMP) > $(TSTOUT) \
	                 $(XLIBSTDERREXTFIX) \
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
