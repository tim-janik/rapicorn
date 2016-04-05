# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_CORE_CFLAGS) $(SVG_DEPENDENCIES_CFLAGS)

# test programs
noinst_PROGRAMS	 = $(TAPTESTS)
LDADDS		 = $(top_builddir)/rcore/librapicorncore-@MAJOR@.la $(SVG_DEPENDENCIES_LIBS)

# == test programs ==
TAPTESTS	  += benchrcore
benchrcore_SOURCES = benchrcore.cc
benchrcore_LDADD   = $(LDADDS)

TAPTESTS          += testrcore1
testrcore1_SOURCES = multitest.cc aidatests.cc markup.cc strings.cc timebits.cc datalist.cc loops.cc
testrcore1_LDADD   = $(LDADDS)
EXTRA_DIST        += data.cc testrcore1.outref

TAPTESTS          += testrcore2
testrcore2_SOURCES = messages.cc xmltest.cc threads.cc svgtool.cc signal2.cc performance.cc
testrcore2_LDADD   = $(LDADDS) $(GDKPIXBUF_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS) $(PANGOCAIRO_LIBS) $(LIBXML_LIBS)
testrcore2_CXXFLAGS= $(GDKPIXBUF_CFLAGS) $(CAIRO_CFLAGS) $(AM_CXXFLAGS) # FIXME
EXTRA_DIST        += testrcore2.outref

export VPATH	# needed by test programs to find builddir relative input files

# == test resources ==
RAPIDRES    = $(abs_builddir)/../rapidres
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
# validate hash reseeding at program start
check-randomized-hashing:
	$(Q) test `./testrcore1 --string-hash64 foo` != `./testrcore1 --string-hash64 foo` ; $(TSTRESULT)
check-local: check-randomized-hashing
# validate failing assertions and fatal aborting
check-assertion-handlers: testrcore2
	$(Q) ( export RAPICORN=debug:no-syslog:no-fatal-syslog ; rm -f $(TSTOUT) ; \
	       for arg in --test-assert --test-unreached --test-fatal --test-TASSERT --test-TCMP --test-assertion-hook ; do \
	         ! ./testrcore2 $$arg 2>>$(TSTOUT) || exit $? ; done ) $(QSTDERR) ; $(TSTRESULT)
	$(Q) grep -q 'failed.*"test-assert"' $(TSTOUT)		; $(TSTRESULT) "check assert"
	$(Q) grep -q 'FATAL:.*not.*reach' $(TSTOUT)		; $(TSTRESULT) "check unreached"
	$(Q) grep -q 'FATAL:.*fatal.*"test-fatal"' $(TSTOUT)	; $(TSTRESULT) "check fatal error"
	$(Q) grep -q 'assert.*fail.*".*TASSERT"' $(TSTOUT)	; $(TSTRESULT) "check TASSERT()"
	$(Q) grep -q 'assert.*fail.*".*TCMP"' $(TSTOUT)		; $(TSTRESULT) "check TCMP()"
	$(Q) grep -q 'assertion-hook.*magic=0xdecaff' $(TSTOUT)	; $(TSTRESULT) "check Test::set_assertion_hook()"
	$(Q) grep -qi 'Aborting..' $(TSTOUT)			; $(TSTRESULT) "check aborting"
	$(Q) rm -f $(TSTOUT)
check-local: check-assertion-handlers
# check various non-fatal logging conditions
check-logging-output: testrcore2
	$(Q) rm -f $(TSTOUT) ; RAPICORN=no-syslog:no-fatal-syslog RAPICORN_DEBUG=all \
	     ./testrcore2 --test-logging 2>>$(TSTOUT)           ; $(TSTRESULT)
	$(Q) grep -q 'debugging messages' $(TSTOUT)		; $(TSTRESULT) "check debugging"
	$(Q) grep -q 'assertion failed: errno == 0' $(TSTOUT)	; $(TSTRESULT) "check errno diagnostics"
	$(Q) grep -q 'failed.*"assert-return-void"' $(TSTOUT)	; $(TSTRESULT) "check assert_return"
	$(Q) grep -q 'failed.*"assert-return+val"' $(TSTOUT)	; $(TSTRESULT) "check assert_return+value"
	$(Q) grep -q 'CRITICAL:.*"test-critical"' $(TSTOUT)	; $(TSTRESULT) "check test critical()"
	$(Q) rm -f $(TSTOUT)
check-local: check-logging-output
# check startup entropy
check-startup-entropy:
	$(Q) A=`./benchrcore --entropy` ; B=`./benchrcore --entropy`; \
	     test -n "$$A$$B" -a "$$A" != "$$B"; $(TSTRESULT)
check-system-entropy:
	$(Q) A=`./benchrcore --system-entropy` ; B=`./benchrcore --system-entropy`; \
	     test -n "$$A$$B" -a "$$A" != "$$B"; $(TSTRESULT)
check-runtime-entropy:
	$(Q) A=`./benchrcore --runtime-entropy` ; B=`./benchrcore --runtime-entropy`; \
	     test -n "$$A$$B" -a "$$A" != "$$B"; $(TSTRESULT)
check-local: check-startup-entropy check-system-entropy check-runtime-entropy
