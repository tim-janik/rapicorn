# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= $(RAPICORN_CORE_CFLAGS) -DRAPICORN_LOG_DOMAIN='"$(basename $(@F))"' -DSRCDIR='"$(srcdir)"'
PLICINT		 = $(top_builddir)/plic/plic-intern

# test programs
noinst_PROGRAMS	 = $(TEST_PROGS) $(NOINSTPRGS)
LDADDS		 = $(top_builddir)/rapicorn-core/librapicorncore@RAPICORN_RELEASE@.la
NOINSTPRGS =

# programs
NOINSTPRGS        += imgcheck
imgcheck_SOURCES   = imgcheck.cc
imgcheck_LDADD     = $(LDADDS)

# test programs
TEST_PROGS        += infotest
infotest_SOURCES   = infotest.cc
infotest_LDADD     = $(LDADDS)
TEST_PROGS        += math
PERFTESTS         += math
math_SOURCES       = math.cc
math_LDADD         = $(LDADDS)
TEST_PROGS        += strings
SLOWTESTS         += strings
strings_SOURCES    = strings.cc
strings_LDADD      = $(LDADDS)
TEST_PROGS        += systest
systest_SOURCES    = systest.cc
systest_LDADD      = $(LDADDS)
TEST_PROGS        += threads
PERFTESTS         += threads
threads_SOURCES    = threads.cc
threads_LDADD      = $(LDADDS)
TEST_PROGS        += signal
signal_SOURCES     = signal.cc
signal_LDADD       = $(LDADDS)
TEST_PROGS        += sorting
SLOWTESTS         += sorting
sorting_SOURCES    = sorting.cc
sorting_LDADD      = $(LDADDS)
TEST_PROGS        += datalist
datalist_SOURCES   = datalist.cc
datalist_LDADD     = $(LDADDS)
EXTRA_DIST        += data.cc
TEST_PROGS        += xmltest
xmltest_SOURCES    = xmltest.cc
xmltest_LDADD      = $(LDADDS)
TEST_PROGS        += markup
markup_SOURCES     = markup.cc
markup_LDADD       = $(LDADDS)
TEST_PROGS        += pixmaps
pixmaps_SOURCES    = pixmaps.cc
pixmaps_LDADD      = $(LDADDS)
EXTRA_DIST        += testpixs.c testpixs.png
TEST_PROGS        += modval
modval_SOURCES     = modval.cc
modval_LDADD       = $(LDADDS)
modval.cc:	     testtypes.tpg
TEST_PROGS        += mod1tst
mod1tst_SOURCES    = mod1tst.cc
mod1tst_LDADD      = $(LDADDS)
mod1tst.cc:          testtypes.tpg

EXTRA_DIST        += testtypes.idl testtypes.tpg

%.tpg: $(srcdir)/%.idl
	$(PLICINT) $< -O TypePackage -o $@
CLEANFILES += *.tpg

# === test rules ===
# process handle
check-process-handle: infotest
	test -n `./infotest --print-process-handle`
	test `./infotest --print-process-handle` != `./infotest --print-process-handle`
check: check-process-handle
