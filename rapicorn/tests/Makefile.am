# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= $(RAPICORN_DEP_CFLAGS) -DRAPICORN_LOG_DOMAIN='"$(basename $(@F))"'
DEFS            += $(GLIB_CFLAGS) # FIXME

# test programs
noinst_PROGRAMS	 = $(TESTS)
noinst_DATA	 =
LDADDS		 = $(top_builddir)/rapicorn/librapicorn@LT_RELEASE@.la $(RAPICORN_DEP_LIBS)
XML_FILES	 =
EXTRA_DIST	+= $(XML_FILES)

# programs
TESTS_ENVIRONMENT  = RAPICORN_VPATH=$(VPATH) # also search srcdir for XML files
TESTS             += walker
walker_SOURCES     = walker.cc
walker_LDADD       = $(LDADDS)
TESTS             += labelmarkup
labelmarkup_SOURCES= labelmarkup.cc
labelmarkup_LDADD  = $(LDADDS)
TESTS             += cmdtest
cmdtest_SOURCES    = cmdtest.cc
cmdtest_LDADD      = $(LDADDS)
TESTS             += properties
properties_SOURCES = properties.cc
properties_LDADD   = $(LDADDS)
TESTS             += primitives
SLOWTESTS         += primitives
primitives_SOURCES = primitives.cc
primitives_LDADD   = $(LDADDS)
TESTS             += region
region_SOURCES     = region.cc
region_LDADD       = $(LDADDS)
TESTS             += shaderect
shaderect_SOURCES  = shaderect.cc
shaderect_LDADD    = $(LDADDS)
TESTS             += pixcombine
pixcombine_SOURCES = pixcombine.cc
pixcombine_LDADD   = $(LDADDS)
TESTS             += items
items_SOURCES      = items.cc
items_LDADD        = $(LDADDS)
TESTS             += testitems
testitems_SOURCES  = testitems.cc
testitems_LDADD    = $(LDADDS)
XML_FILES         += testitems.xml
TESTS             += factory
factory_SOURCES    = factory.cc
factory_LDADD      = $(LDADDS)
XML_FILES         += factory.xml
TESTS             += evaltest
evaltest_SOURCES   = evaltest.cc
evaltest_LDADD     = $(LDADDS)

xmllint-check:
	cd $(srcdir) && $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
