# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES += -I$(top_srcdir) $(RAPICORN_CORE_CFLAGS)
DEFS	 += -DRAPICORN_CONVENIENCE -D__SOURCE_COMPONENT__='"Rapicorn@RAPICORN_RELEASE@/$(subdir)/$<"'

# test programs
noinst_PROGRAMS	 = $(TEST_PROGS) $(LOGTEST_PROGS)
LDADDS		 = $(top_builddir)/rcore/librapicorncore@RAPICORN_RELEASE@.la

# == test programs ==
LOGTEST_PROGS     += testrcore1
TEST_PROGS        += testrcore1
testrcore1_SOURCES = multitest.cc markup.cc strings.cc timebits.cc datalist.cc loops.cc
testrcore1_LDADD   = $(LDADDS)
EXTRA_DIST        += data.cc testrcore1.ref

LOGTEST_PROGS     += testrcore2
TEST_PROGS        += testrcore2
testrcore2_SOURCES = messages.cc xmltest.cc threads.cc svgtool.cc signal.cc performance.cc
testrcore2_LDADD   = $(LDADDS) $(top_builddir)/rcore/rsvg/librcore_rsvg@RAPICORN_RELEASE@.o \
		     $(GDKPIXBUF_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS) $(PANGOCAIRO_LIBS) $(LIBXML_LIBS)
testrcore2_CXXFLAGS= $(GDKPIXBUF_CFLAGS) $(CAIRO_CFLAGS) # FIXME
EXTRA_DIST        += testrcore2.ref

export VPATH	# needed by test programs to find builddir relative input files

# == test resources ==
ZINTERN     = $(abs_builddir)/../rapicorn-zintern
RES_FILES   = sample1.svg
RES_PAIRS   = $(foreach file, $(RES_FILES), test-files/$(notdir $(file)) $(file))
EXTRA_DIST += $(RES_FILES)
resources.cc: $(RES_FILES) $(ZINTERN) Makefile.am
	$(AM_V_GEN)
	$(Q) cd $(srcdir) && $(ZINTERN) -r $(RES_PAIRS) > $(abs_builddir)/xgen-$(@F)
	$(Q) mv xgen-$(@F) $@
EXTRA_DIST  += resources.cc
CLEANFILES  += resources.cc
noinst_DATA  = resources.cc
svgtool.cc: resources.cc

# === test rules ===
# validate failing assertions and fatal aborting
check-assertion-handlers: testrcore2
	$(Q) ( export RAPICORN=debug:no-syslog:no-fatal-syslog ; rm -f $(TSTOUT) ; \
	       for arg in --test-assert --test-unreached --test-fatal --test-TCMP ; do \
	         ! ./testrcore2 $$arg 2>>$(TSTOUT) || exit $? ; done ) $(QSTDERR) ; eval "$$TSTDIAGNOSE"
	$(Q) grep -q 'failed.*"test-assert"' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) failing assert'"
	$(Q) grep -q 'FATAL:.*not.*reach' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) failing unreached'"
	$(Q) grep -q 'FATAL:.*fatal.*"test-fatal"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) fatal error'"
	$(Q) grep -q 'FATAL:.*".*fail.*TCMP"' $(TSTOUT)	        ; eval "$$TSTDIAGNOSE" "'$(@F) failing TCMP()'"
	$(Q) grep -qi 'Aborting..' $(TSTOUT)			; eval "$$TSTDIAGNOSE" "'$(@F) aborting'"
	$(Q) rm -f $(TSTOUT)
check: check-assertion-handlers
# check various non-fatal logging conditions
check-logging-output: testrcore2
	$(Q) rm -f $(TSTOUT) ; RAPICORN=debug:no-syslog:no-fatal-syslog \
	     ./testrcore2 --test-logging 2>>$(TSTOUT)           ; eval "$$TSTDIAGNOSE"
	$(Q) grep -q '(debugging message)' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) debugging'"
	$(Q) grep -q 'assertion failed: errno == 0' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) errno diagnostics'"
	$(Q) grep -q 'exception.*"throw-if-fail"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) throw-if-fail'"
	$(Q) grep -q 'failed.*"assert-return-void"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) assert_return'"
	$(Q) grep -q 'failed.*"assert-return+val"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) assert_return+value'"
	$(Q) grep -q 'CRITICAL:.*"test-critical"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) test critical()'"
	$(Q) grep -q 'FIX''ME:.*FIX''ME() test' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) test FIX''ME()'"
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
