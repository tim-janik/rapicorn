# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= $(RAPICORN_CFLAGS) -DRAPICORN_LOG_DOMAIN='"$(basename $(@F))"'
DEFS		+= $(GLIB_CFLAGS) # FIXME

# test programs
noinst_PROGRAMS	 = $(TESTS)
LDADDS		 = $(top_builddir)/rapicorn-core/librapicorncore@LT_RELEASE@.la $(RAPICORN_LIBS)

# programs
TESTS             += infotest
infotest_SOURCES   = infotest.cc
infotest_LDADD     = $(LDADDS)
TESTS             += math
PERFTESTS         += math
math_SOURCES       = math.cc
math_LDADD         = $(LDADDS)
TESTS             += strings
SLOWTESTS         += strings
strings_SOURCES    = strings.cc
strings_LDADD      = $(LDADDS)
TESTS             += systest
systest_SOURCES    = systest.cc
systest_LDADD      = $(LDADDS)
TESTS             += threads
PERFTESTS         += threads
threads_SOURCES    = threads.cc
threads_LDADD      = $(LDADDS)
TESTS             += signal
signal_SOURCES     = signal.cc
signal_LDADD       = $(LDADDS)
TESTS             += sorting
SLOWTESTS         += sorting
sorting_SOURCES    = sorting.cc
sorting_LDADD      = $(LDADDS)
TESTS             += datalist
datalist_SOURCES   = datalist.cc
datalist_LDADD     = $(LDADDS)
EXTRA_DIST        += data.cc
TESTS             += xmltest
xmltest_SOURCES    = xmltest.cc
xmltest_LDADD      = $(LDADDS)
TESTS             += markup
markup_SOURCES     = markup.cc
markup_LDADD       = $(LDADDS)
TESTS             += pixmaps
pixmaps_SOURCES    = pixmaps.cc
pixmaps_LDADD      = $(LDADDS)
