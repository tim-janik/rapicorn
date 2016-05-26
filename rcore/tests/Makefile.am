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

# === test rules ===
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
