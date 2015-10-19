# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_CORE_CFLAGS)

# test programs
noinst_PROGRAMS	 = $(TEST_PROGS) $(LOGTEST_PROGS)
LDADDS		 = $(top_builddir)/rcore/librapicorncore-@MAJOR@.la

# == test programs ==
LOGTEST_PROGS     += testrcore1
TEST_PROGS        += testrcore1
testrcore1_SOURCES = multitest.cc aidatests.cc markup.cc strings.cc timebits.cc datalist.cc loops.cc
testrcore1_LDADD   = $(LDADDS)
EXTRA_DIST        += data.cc testrcore1.ref

LOGTEST_PROGS     += testrcore2
TEST_PROGS        += testrcore2
testrcore2_SOURCES = messages.cc xmltest.cc threads.cc svgtool.cc signal2.cc performance.cc
testrcore2_LDADD   = $(LDADDS) $(top_builddir)/rcore/rsvg/librapicorn-rsvg-@MAJOR@.la \
		     $(GDKPIXBUF_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS) $(PANGOCAIRO_LIBS) $(LIBXML_LIBS)
testrcore2_CXXFLAGS= $(GDKPIXBUF_CFLAGS) $(CAIRO_CFLAGS) $(AM_CXXFLAGS) # FIXME
EXTRA_DIST        += testrcore2.ref

export VPATH	# needed by test programs to find builddir relative input files

# == test resources ==
RAPIDRES    = $(abs_builddir)/../rapidres-@MAJOR@
RES_FILES   = sample1.svg
EXTRA_DIST += $(RES_FILES)
resources.cc: $(RES_FILES) $(RAPIDRES) Makefile.am
	$(AM_V_GEN)
	$(Q) cd $(srcdir) && $(RAPIDRES) $(RES_FILES) > $(abs_builddir)/xgen-$(@F)
	$(Q) mv xgen-$(@F) $@
EXTRA_DIST  += resources.cc
CLEANFILES  += resources.cc
noinst_DATA  = resources.cc
svgtool.cc: resources.cc

# === test rules ===
# validate failing assertions and fatal aborting
check-assertion-handler: testrcore2
	$(Q) ( export RAPICORN=debug:no-syslog:no-fatal-syslog ; rm -f $(TSTOUT) ; \
	       for arg in --test-assert --test-unreached --test-fatal --test-TASSERT --test-TCMP --test-assertion-hook ; do \
	         ! ./testrcore2 $$arg 2>>$(TSTOUT) || exit $? ; done ) $(QSTDERR) ; eval "$$TSTDIAGNOSE"
	$(Q) grep -q 'failed.*"test-assert"' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) assert'"
	$(Q) grep -q 'FATAL:.*not.*reach' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) unreached'"
	$(Q) grep -q 'FATAL:.*fatal.*"test-fatal"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) fatal error'"
	$(Q) grep -q 'assert.*fail.*".*TASSERT"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) TASSERT()'"
	$(Q) grep -q 'assert.*fail.*".*TCMP"' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) TCMP()'"
	$(Q) grep -q 'assertion-hook.*magic=0xdecaff' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) Test::set_assertion_hook()'"
	$(Q) grep -qi 'Aborting..' $(TSTOUT)			; eval "$$TSTDIAGNOSE" "'$(@F) aborting'"
	$(Q) rm -f $(TSTOUT)
check: check-assertion-handler
# check various non-fatal logging conditions
check-logging-output: testrcore2
	$(Q) rm -f $(TSTOUT) ; RAPICORN=no-syslog:no-fatal-syslog RAPICORN_DEBUG=all \
	     ./testrcore2 --test-logging 2>>$(TSTOUT)           ; eval "$$TSTDIAGNOSE"
	$(Q) grep -q 'debugging messages' $(TSTOUT)		; eval "$$TSTDIAGNOSE" "'$(@F) debugging'"
	$(Q) grep -q 'assertion failed: errno == 0' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) errno diagnostics'"
	$(Q) grep -q 'failed.*"assert-return-void"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) assert_return'"
	$(Q) grep -q 'failed.*"assert-return+val"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) assert_return+value'"
	$(Q) grep -q 'CRITICAL:.*"test-critical"' $(TSTOUT)	; eval "$$TSTDIAGNOSE" "'$(@F) test critical()'"
	$(Q) rm -f $(TSTOUT)
check: check-logging-output
# process handle
check-process-handle: testrcore1
	$(Q) test -n `./testrcore1 --print-process-handle`; eval "$$TSTDIAGNOSE"
check: check-process-handle
check-unique-process: testrcore1
	$(Q) test `./testrcore1 --print-process-handle` != `./testrcore1 --print-process-handle`; eval "$$TSTDIAGNOSE"
check: check-unique-process
# check startup entropy
check-startup-entropy:
	$(Q) A=`./testrcore2 --entropy` ; B=`./testrcore2 --entropy`; \
	     test -n "$$A$$B" -a "$$A" != "$$B"; eval "$$TSTDIAGNOSE"
check-system-entropy:
	$(Q) A=`./testrcore2 --system-entropy` ; B=`./testrcore2 --system-entropy`; \
	     test -n "$$A$$B" -a "$$A" != "$$B"; eval "$$TSTDIAGNOSE"
check-runtime-entropy:
	$(Q) A=`./testrcore2 --runtime-entropy` ; B=`./testrcore2 --runtime-entropy`; \
	     test -n "$$A$$B" -a "$$A" != "$$B"; eval "$$TSTDIAGNOSE"
check: check-startup-entropy check-system-entropy check-runtime-entropy
