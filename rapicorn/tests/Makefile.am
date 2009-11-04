# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= $(RAPICORN_GUI_CFLAGS) -DRAPICORN_LOG_DOMAIN='"$(basename $(@F))"'

# test programs
noinst_PROGRAMS	 = $(TEST_PROGS)
noinst_DATA	 =
LDADDS		 = $(top_builddir)/rapicorn/librapicorn@RAPICORN_RELEASE@.la
XML_FILES	 =
EXTRA_DIST	+= $(XML_FILES)

# programs
TESTS_ENVIRONMENT  = RAPICORN_VPATH=$(VPATH) # also search srcdir for XML files
TEST_PROGS        += walker
walker_SOURCES     = walker.cc
walker_LDADD       = $(LDADDS)
TEST_PROGS        += labelmarkup
labelmarkup_SOURCES= labelmarkup.cc
labelmarkup_LDADD  = $(LDADDS)
TEST_PROGS        += cmdtest
cmdtest_SOURCES    = cmdtest.cc
cmdtest_LDADD      = $(LDADDS)
TEST_PROGS        += properties
properties_SOURCES = properties.cc
properties_LDADD   = $(LDADDS)
TEST_PROGS        += primitives
SLOWTESTS         += primitives
primitives_SOURCES = primitives.cc
primitives_LDADD   = $(LDADDS)
TEST_PROGS        += region
region_SOURCES     = region.cc
region_LDADD       = $(LDADDS)
TEST_PROGS        += shaderect
shaderect_SOURCES  = shaderect.cc
shaderect_LDADD    = $(LDADDS)
TEST_PROGS        += sinfextest
sinfextest_SOURCES = sinfextest.cc
sinfextest_LDADD   = $(LDADDS) $(READLINELIBS)
TEST_PROGS        += pixcombine
pixcombine_SOURCES = pixcombine.cc
pixcombine_LDADD   = $(LDADDS)
TEST_PROGS        += items
items_SOURCES      = items.cc
items_LDADD        = $(LDADDS)
TEST_PROGS        += testitems
testitems_SOURCES  = testitems.cc
testitems_LDADD    = $(LDADDS)
XML_FILES         += testitems.xml
TEST_PROGS        += factory
factory_SOURCES    = factory.cc
factory_LDADD      = $(LDADDS)
XML_FILES         += factory.xml

SINFEX=./sinfextest --shell
sinfex-expressions:
	@$(call TSTTITLE)
	$(_@) egrep -v '^[[:space:]]*(#|$$)' < $(srcdir)/expressions.txt > $(TSTTMP)
	$(_@) $(XVFB_START) && \
	  COLUMNS=9999 $(SINFEX) < $(TSTTMP) > $(TSTOUT) \
	                 $(XLIBSTDERREXTFIX)
	@$(call TSTCMP, expressions.ref)
	$(_@) rm -f $(TSTTMP)
check-am: sinfex-expressions

xmllint-check:
	cd $(srcdir) && $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES  += stamp-autochecks
