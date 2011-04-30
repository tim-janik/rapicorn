# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir) $(RAPICORN_CORE_CFLAGS)
DEFS		+= -DRAPICORN_LOG_DOMAIN='"$(basename $(@F))"' -DRAPICORN_CONVENIENCE -DSRCDIR='"$(srcdir)"'
PLICINT		 = $(top_builddir)/plic/plic-intern

# test programs
noinst_PROGRAMS	 = $(TEST_PROGS) $(LOGTEST_PROGS) $(NOINSTPRGS)
LDADDS		 = $(top_builddir)/rcore/librapicorncore@RAPICORN_RELEASE@.la
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
	$(PLICINT) $< -G TypePackage -o $@
CLEANFILES += *.tpg

# logfile tests
LOGTEST_PROGS     += svgtool
svgtool_SOURCES    = svgtool.cc
svgtool_LDADD      = $(LDADDS) $(top_builddir)/rcore/rsvg/librcore_rsvg@RAPICORN_RELEASE@.o \
	$(GDKPIXBUF_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS) $(PANGOCAIRO_LIBS) $(LIBXML_LIBS)
svgtool_CXXFLAGS   = $(GDKPIXBUF_CFLAGS) $(CAIRO_CFLAGS) # FIXME
LOGTEST_PROGS     += datalist
datalist_SOURCES   = datalist.cc
datalist_LDADD     = $(LDADDS)
LOGTEST_PROGS     += messages
messages_SOURCES   = messages.cc
messages_LDADD     = $(LDADDS)


# === test rules ===
# simple logging check
check-logging-simple: messages
	@$(call TSTTITLE, Simple logging function check)
	$(_@) ( RAPICORN=diag:stable:debug:no-syslog:no-fatal-syslog \
	        ./messages --test-logging 2>$(TSTOUT) ; true ) 2>/dev/null
	$(_@) grep -q 'debug-.*logging test' $(TSTOUT) || $(call TSTRESULT)
	$(_@) grep -q 'diagnostic.*errno' $(TSTOUT) || $(call TSTRESULT)
	$(_@) grep -q 'WARNING:.*logging message' $(TSTOUT) || $(call TSTRESULT)
	$(_@) grep -q 'ERROR:.*serious' $(TSTOUT) || $(call TSTRESULT)
	$(_@) grep -q 'FATAL:.*abort' $(TSTOUT) || $(call TSTRESULT)
	$(_@) grep -qi 'aborting..' $(TSTOUT) || $(call TSTRESULT)
	$(_@) $(call TSTRESULT)
	$(_@) rm -f $(TSTOUT)
check: check-logging-simple
# process handle
check-process-handle: infotest
	@$(call TSTTITLE, Simple process-handle check)
	$(_@) test -n `./infotest --print-process-handle`; $(call TSTRESULT)
	@$(call TSTTITLE, Unique process-handle test)
	$(_@) test `./infotest --print-process-handle` != `./infotest --print-process-handle`; $(call TSTRESULT)
check: check-process-handle
# process handle
check-locatable-id: infotest
	@$(call TSTTITLE, Simple locatable-id check)
	$(_@) test -n `./infotest --print-locatable-id`; $(call TSTRESULT)
	@$(call TSTTITLE, Unique locatable-id test)
	$(_@) test `./infotest --print-locatable-id` != `./infotest --print-locatable-id`; $(call TSTRESULT)
check: check-locatable-id
