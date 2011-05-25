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
TEST_PROGS        += multitest
multitest_SOURCES  = multitest.cc
multitest_LDADD    = $(LDADDS)
TEST_PROGS        += strings
SLOWTEST_PROGS    += strings
strings_SOURCES    = strings.cc
strings_LDADD      = $(LDADDS)
TEST_PROGS        += performance
performance_SOURCES= performance.cc
performance_LDADD  = $(LDADDS)
TEST_PROGS        += threads
PERFTESTS         += threads
threads_SOURCES    = threads.cc
threads_LDADD      = $(LDADDS)
TEST_PROGS        += signal
signal_SOURCES     = signal.cc
signal_LDADD       = $(LDADDS)
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
check-logging-output: messages
	$(Q) ( RAPICORN=diag:stable:debug:no-syslog:no-fatal-syslog \
	        ./messages --test-logging 2>$(TSTOUT) ; true ) $(QSTDERR) ; eval "$$TSTDIAGNOSE"
	$(Q) grep -q 'debug-.*logging test' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) debugging'"
	$(Q) grep -q 'diagnostic.*errno' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) errno diagnostics'"
	$(Q) grep -q 'WARNING:.*logging message' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) warnings'"
	$(Q) grep -q 'ERROR:.*serious' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) errors'"
	$(Q) grep -q 'FATAL:.*abort' $(TSTOUT)			; eval "$$TSTDIAGNOSE" "'$(@F) fatals'"
	$(Q) grep -qi 'aborting..' $(TSTOUT)			; eval "$$TSTDIAGNOSE" "'$(@F) aborting'"
	$(Q) rm -f $(TSTOUT)
check: check-logging-output
# process handle
check-process-handle: multitest
	$(Q) test -n `./multitest --print-process-handle`; eval "$$TSTDIAGNOSE"
check: check-process-handle
check-unique-process: multitest
	$(Q) test `./multitest --print-process-handle` != `./multitest --print-process-handle`; eval "$$TSTDIAGNOSE"
check: check-unique-process

# process handle
check-locatable-id: multitest
	$(Q) test -n `./multitest --print-locatable-id`; eval "$$TSTDIAGNOSE"
check: check-locatable-id
check-unique-id: multitest
	$(Q) test `./multitest --print-locatable-id` != `./multitest --print-locatable-id`; eval "$$TSTDIAGNOSE"
check: check-unique-id
