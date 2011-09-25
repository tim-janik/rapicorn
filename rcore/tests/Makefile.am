# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) $(RAPICORN_CORE_CFLAGS)
DEFS	 += -DRAPICORN_CONVENIENCE -D__SOURCE_FILE__='"$(patsubst $(realpath $(abs_top_srcdir))/%,%, $(realpath $<))"'
PLICINT	  = $(top_builddir)/plic/plic-intern

# test programs
noinst_PROGRAMS	 = $(TEST_PROGS) $(LOGTEST_PROGS) $(NOINSTPRGS)
LDADDS		 = $(top_builddir)/rcore/librapicorncore@RAPICORN_RELEASE@.la
NOINSTPRGS =

# programs
NOINSTPRGS        += imgcheck
imgcheck_SOURCES   = imgcheck.cc
imgcheck_LDADD     = $(LDADDS)

# test programs
LOGTEST_PROGS     += testrcore1
TEST_PROGS        += testrcore1
testrcore1_SOURCES = multitest.cc markup.cc strings.cc datalist.cc loops.cc
testrcore1_LDADD   = $(LDADDS)
EXTRA_DIST        += data.cc

LOGTEST_PROGS     += testrcore2
TEST_PROGS        += testrcore2
testrcore2_SOURCES = messages.cc xmltest.cc threads.cc svgtool.cc signal.cc pixmaps.cc performance.cc
testrcore2_LDADD   = $(LDADDS) $(top_builddir)/rcore/rsvg/librcore_rsvg@RAPICORN_RELEASE@.o \
		     $(GDKPIXBUF_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS) $(PANGOCAIRO_LIBS) $(LIBXML_LIBS)
testrcore2_CXXFLAGS= $(GDKPIXBUF_CFLAGS) $(CAIRO_CFLAGS) # FIXME
EXTRA_DIST        += testpixs.c testpixs.png

EXTRA_DIST        += testtypes.idl


# === test rules ===
# simple logging check
check-logging-output: testrcore2
	$(Q) ( export RAPICORN=debug:no-syslog:no-fatal-syslog ; rm -f $(TSTOUT) ; \
	       for arg in --test-assert --test-passert --test-unreached --test-fatal --test-pfatal --test-logging ; do \
	         ./testrcore2 $$arg 2>>$(TSTOUT) ; done ; true ) $(QSTDERR) ; eval "$$TSTDIAGNOSE"
	$(Q) grep -q 'failed.*"test-assert"' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) test-assert'"
	$(Q) grep -q 'failed.*"test-passert"' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) test-passert'"
	$(Q) grep -q 'FATAL:.*unreach' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) test-unreached'"
	$(Q) grep -q 'FATAL:.*fatal.*"test-fatal"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) test-fatal'"
	$(Q) grep -q 'FATAL:.*"test-pfatal"' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) test-pfatal'"
	$(Q) grep -q '(debugging message)' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) debugging'"
	$(Q) grep -q ':.*random.*"test-pdebug"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) test-pdebug'"
	$(Q) grep -q 'check failed (errno == 0)' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) errno diagnostics'"
	$(Q) grep -q 'exception.*"throw-if-fail"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) throw-if-fail'"
	$(Q) grep -q 'failed.*"test-return-if-fail"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) return-if-fail'"
	$(Q) grep -q 'failed.*"test-return-val-if-' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) return-val-if-fail'"
	$(Q) grep -q 'CRITICAL:.*"test-critical"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) test-critical'"
	$(Q) grep -q 'CRITICAL:.*"test-pcritical"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) test-pcritical'"
	$(Q) grep -qi 'Aborting..' $(TSTOUT)			; eval "$$TSTDIAGNOSE" "'$(@F) aborting'"
	$(Q) rm -f $(TSTOUT)
check: check-logging-output
# process handle
check-process-handle: testrcore1
	$(Q) test -n `./testrcore1 --print-process-handle`; eval "$$TSTDIAGNOSE"
check: check-process-handle
check-unique-process: testrcore1
	$(Q) test `./testrcore1 --print-process-handle` != `./testrcore1 --print-process-handle`; eval "$$TSTDIAGNOSE"
check: check-unique-process

# process handle
check-locatable-id: testrcore1
	$(Q) test -n `./testrcore1 --print-locatable-id`; eval "$$TSTDIAGNOSE"
check: check-locatable-id
check-unique-id: testrcore1
	$(Q) test `./testrcore1 --print-locatable-id` != `./testrcore1 --print-locatable-id`; eval "$$TSTDIAGNOSE"
check: check-unique-id
