# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_GUI_CFLAGS)

noinst_PROGRAMS  = benchaida $(EXAMPLES)
noinst_DATA      =
XML_FILES        =
EXAMPLES         =
EXTRA_DIST      += $(XML_FILES)

LDADDS   = $(top_builddir)/ui/librapicorn-@MAJOR@.la $(RAPICORN_DEP_LIBS)
AIDACC_INTERN = $(top_srcdir)/aidacc/aidacc-intern
CXXSTUB_PY    = $(top_srcdir)/aidacc/CxxStub.py
RAPIDRES    = $(top_builddir)/rcore/rapidres
RAPIDRUN = $(top_builddir)/tools/rapidrun
IMGCHECK = $(top_builddir)/ui/tests/imgcheck
X11TEST_RAPIDRUN = $(X11_ENV) $(RAPIDRUN) --fatal-warnings "$$@"
X11_TESTPYTHON = $(X11_ENV) PYTHONPATH="$(abs_top_builddir)/cython/${objdir}:$$PYTHONPATH" $(PYTHON)
LIBRAPICORN_LA = $(top_builddir)/rcore/librapicorncore-@MAJOR@.la

# Loading newly built Python modules requires accesss to libtool objdir
objdir       = .libs

# == benchaida ==
benchaida_SOURCES  = benchaida.cc
benchaida_LDADD    = $(LDADDS)


# == t201-rcore-basics ==
t201-rcore-basics-test: t201-rcore-basics
	$(Q) $(top_srcdir)/taptool.sh --test-name t201-rcore-basics -- ./t201-rcore-basics
noinst_PROGRAMS           += t201-rcore-basics
t201_rcore_basics_LDADD    = $(LIBRAPICORN_LA)
t201_rcore_basics_SOURCES  = t201-rcore-basics-main.cc
t201_rcore_basics_SOURCES += t201-rcore-basics-aidatests.cc
t201_rcore_basics_SOURCES += t201-rcore-basics-markup.cc
t201_rcore_basics_SOURCES += t201-rcore-basics-strings.cc
t201_rcore_basics_SOURCES += t201-rcore-basics-timebits.cc
t201_rcore_basics_SOURCES += t201-rcore-basics-datalist.cc
t201_rcore_basics_SOURCES += t201-rcore-basics-loops.cc
t201_rcore_basics_DEPENDENCIES = t201-rcore-basics-xmldata.cc
check: t201-rcore-basics-test

# == t202-randomized-hashing ==
t202-randomized-hashing-test: t201-rcore-basics	# test hash reseeding at program start
	$(Q) test `./t201-rcore-basics --string-hash64 foo` != `./t201-rcore-basics --string-hash64 foo` ; $(TSTRESULT)
check: t202-randomized-hashing-test

# == t203-more-basics ==
t203-more-basics-test: t203-more-basics
	$(Q) $(top_srcdir)/taptool.sh --test-name t203-more-basics -- ./t203-more-basics
t203-more-basics-resources.cc: $(RAPIDRES) $(t203_more_basics_resfiles)
	$(AM_V_GEN)
	$(Q) $(RAPIDRES) $(t203_more_basics_resfiles:%=$(srcdir)/%) > $@.xtmp
	$(Q) mv $@.xtmp $@
t203_more_basics_resfiles = t203-more-basics-sample.svg
EXTRA_DIST += $(t203_more_basics_resfiles)
CLEANFILES += t203-more-basics-resources.cc
noinst_PROGRAMS           += t203-more-basics
t203_more_basics_LDADD    = $(LIBRAPICORN_LA) $(SVG_DEPENDENCIES_LIBS)
t203_more_basics_SOURCES  = t203-more-basics-main.cc
t203_more_basics_SOURCES += t203-more-basics-performance.cc
t203_more_basics_SOURCES += t203-more-basics-signal2.cc
t203_more_basics_SOURCES += t203-more-basics-svgtool.cc
t203_more_basics_SOURCES += t203-more-basics-threads.cc
t203_more_basics_SOURCES += t203-more-basics-xmltest.cc
t203-more-basics-svgtool.cc: t203-more-basics-resources.cc
t203-more-basics-xmltest.cc: t201-rcore-basics-xmldata.cc
check: t203-more-basics-test

# == t204-assertions ==
t204-assertions-test: t203-more-basics # validate failing assertions and fatal aborting
	$(Q) rm -f $(TSTOUT)
	$(Q) (! $(t204_assertions_env) ./t203-more-basics --test-assert		2>> $(TSTOUT)) $(QSTDERR)
	$(Q) (! $(t204_assertions_env) ./t203-more-basics --test-unreached	2>> $(TSTOUT)) $(QSTDERR)
	$(Q) (! $(t204_assertions_env) ./t203-more-basics --test-fatal		2>> $(TSTOUT)) $(QSTDERR)
	$(Q) (! $(t204_assertions_env) ./t203-more-basics --test-TASSERT	2>> $(TSTOUT)) $(QSTDERR)
	$(Q) (! $(t204_assertions_env) ./t203-more-basics --test-TCMP		2>> $(TSTOUT)) $(QSTDERR)
	$(Q) (! $(t204_assertions_env) ./t203-more-basics --test-assertion-hook	2>> $(TSTOUT)) $(QSTDERR)
	$(Q) grep -q 'failed.*"test-assert"'          $(TSTOUT) ; $(TSTRESULT) "check assert"
	$(Q) grep -q 'FATAL:.*not.*reach'             $(TSTOUT) ; $(TSTRESULT) "check unreached"
	$(Q) grep -q 'FATAL:.*fatal.*"test-fatal"'    $(TSTOUT) ; $(TSTRESULT) "check fatal error"
	$(Q) grep -q 'assert.*fail.*".*TASSERT"'      $(TSTOUT) ; $(TSTRESULT) "check TASSERT()"
	$(Q) grep -q 'assert.*fail.*".*TCMP"'         $(TSTOUT) ; $(TSTRESULT) "check TCMP()"
	$(Q) grep -q 'assertion-hook.*magic=0xdecaff' $(TSTOUT) ; $(TSTRESULT) "check Test::set_assertion_hook()"
	$(Q) grep -qi 'Aborting..'                    $(TSTOUT) ; $(TSTRESULT) "check aborting"
	$(Q) eval "$$TSTDIAGNOSE"
	$(Q) rm -f $(TSTOUT)
t204_assertions_env = RAPICORN=debug:no-syslog:no-fatal-syslog
check: t204-assertions-test

# == t303-mini-server ==
t303-mini-server-test: t303-mini-server
	$(Q) ./t303-mini-server ; eval "$$TSTDIAGNOSE"
EXTRA_DIST += t303-mini-server.idl t303-mini-server-impl.cc
T303_GENERATED = t303-mini-server-srvt.hh t303-mini-server-srvt.cc t303-mini-server-clnt.hh t303-mini-server-clnt.cc
# MAKE(1) supports parallel execution of a multiple-targets-rule *only* for implicit rules
t303-%-clnt.hh t303-%-clnt.cc t303-%-srvt.hh t303-%-srvt.cc: t303-%.idl $(CXXSTUB_PY) $(AIDACC_INTERN)
	$(AM_V_GEN)
	$(Q) $(AIDACC_INTERN) -x CxxStub -G clienthh -G strip-path=$(abs_top_srcdir)/ $< -o t303-$*-clnt.hh
	$(Q) $(AIDACC_INTERN) -x CxxStub -G clientcc -G strip-path=$(abs_top_srcdir)/ $< -o t303-$*-clnt.cc
	$(Q) $(AIDACC_INTERN) -x CxxStub -G serverhh -G strip-path=$(abs_top_srcdir)/ $< -o t303-$*-srvt.hh
	$(Q) $(AIDACC_INTERN) -x CxxStub -G servercc -G strip-path=$(abs_top_srcdir)/ $< -o t303-$*-srvt.cc
CLEANFILES += $(T303_GENERATED)
t303-mini-server-main.cc: $(T303_GENERATED)
t303_mini_server_SOURCES = t303-mini-server-main.cc
t303_mini_server_LDADD   = $(LIBRAPICORN_LA)
noinst_PROGRAMS         += t303-mini-server
check: t303-mini-server-test

# == t500-main-loop ==
t500-main-loop-test:
	$(Q) $(X11_TESTPYTHON) t500-main-loop.py ; eval "$$TSTDIAGNOSE"
EXTRA_DIST += t500-main-loop.py
check: t500-main-loop-test

# == t501-simple-click ==
t501-simple-click-test:
	$(Q) $(X11_TESTPYTHON) t501-simple-click.py ; eval "$$TSTDIAGNOSE"
EXTRA_DIST += t501-simple-click.py
check: t501-simple-click-test

# == t502-property-basics ==
t502-property-basics-test:
	$(Q) $(X11_TESTPYTHON) t502-property-basics.py ; eval "$$TSTDIAGNOSE"
EXTRA_DIST += t502-property-basics.py
check: t502-property-basics-test

# == t503-garbage-collector ==
t503-garbage-collector-test:
	$(Q) $(X11_TESTPYTHON) t503-garbage-collector.py ; eval "$$TSTDIAGNOSE"
EXTRA_DIST += t503-garbage-collector.py
check: t503-garbage-collector-test

# == t504-size-groups ==
t504-size-groups-test:
	$(Q) $(X11_TESTPYTHON) t504-size-groups.py ; eval "$$TSTDIAGNOSE"
EXTRA_DIST += t504-size-groups.py
check: t504-size-groups-test

# == t505-property-binding ==
t505-property-binding-test:
	$(Q) $(X11_TESTPYTHON) t505-property-binding.py ; eval "$$TSTDIAGNOSE"
EXTRA_DIST += t505-property-binding.py
check: t505-property-binding-test

# == t600-xmlui-eval ==
t600-xmlui-eval-test:
	$(Q) $(X11TEST_RAPIDRUN) -x $(srcdir)/t600-xmlui-eval.xml ; eval "$$TSTDIAGNOSE"
XML_FILES += t600-xmlui-eval.xml
check: t600-xmlui-eval-test

# == t601-arrow-sizing ==
t601-arrow-sizing-test:
	$(Q) $(X11TEST_RAPIDRUN) -x $(srcdir)/t601-arrow-sizing.xml ; eval "$$TSTDIAGNOSE"
EXTRA_DIST += t601-arrow-sizing.xml
check: t601-arrow-sizing-test

# == t602-factory-dump ==
t602-factory-dump-test: $(srcdir)/t602-factory-dump.ref
	$(Q) $(X11TEST_RAPIDRUN) -x --test-dump \
	  --test-matched-node=".*" $(srcdir)/t602-factory-dump.xml > $(TSTOUT) ; eval "$$TSTDIAGNOSE"
	$(Q) eval "$$TSTDIFF" "'$(srcdir)/t602-factory-dump.ref'" "'$(TSTOUT)'"
EXTRA_DIST += t602-factory-dump.xml t602-factory-dump.ref
check: t602-factory-dump-test

# == t700-png-image ==
t700-png-image-test: $(srcdir)/t700-png-image.xml $(srcdir)/t700-png-image.png # $(IMGCHECK) $(X11TEST_RAPIDRUN)
	$(Q) rm -f t700-png-image-snap.png
	$(Q) test $(srcdir) = $(builddir) || cp $(srcdir)/t700-png-image.png $(builddir)
	$(Q) export RAPIDRUN_RES=./ ; $(X11TEST_RAPIDRUN) -x $(srcdir)/t700-png-image.xml
	$(Q) test $(srcdir) = $(builddir) || rm -f $(builddir)/t700-png-image.png
	$(Q) $(IMGCHECK) $(srcdir)/t700-png-image.png t700-png-image-snap.png ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f t700-png-image-snap.png
XML_FILES  += t700-png-image.xml
EXTRA_DIST += t700-png-image.png
check: t700-png-image-test

# == t701-alignment-layout ==
t701-alignment-layout-test: $(srcdir)/t701-alignment-layout.xml $(srcdir)/t701-alignment-layout.png
	$(Q) rm -f t701-alignment-layout-snap.png
	$(Q) $(X11TEST_RAPIDRUN) -x $(srcdir)/t701-alignment-layout.xml
	$(Q) $(IMGCHECK) $(srcdir)/t701-alignment-layout.png t701-alignment-layout-snap.png ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f t701-alignment-layout-snap.png
XML_FILES  += t701-alignment-layout.xml
EXTRA_DIST += t701-alignment-layout.png
check: t701-alignment-layout-test

# == t702-text-ocr ==
t702-text-ocr-test: $(srcdir)/t702-text-ocr.xml
	$(Q) rm -f t702-text-ocr-snap.png
	$(Q) $(X11TEST_RAPIDRUN) --snapshot=t702-text-ocr-snap.png -x $(srcdir)/t702-text-ocr.xml $(QSTDERR)
	$(Q) eval "$$TSTOCR" t702-text-ocr-snap.png "Regu.ar.Text" \""Insensitive Text"\" \""Editab.e Text Edit.r Text"\"
	$(Q) rm -f t702-text-ocr-snap.png
XML_FILES  += t702-text-ocr.xml
check: t702-text-ocr-test

# == t800-binding-example ==
t800-binding-example-test:
	$(Q) $(X11_TESTPYTHON) t800-binding-example.py ; eval "$$TSTDIAGNOSE"
EXTRA_DIST += t800-binding-example.py
check: t800-binding-example-test


xmllint-check:
	$(Q) cd $(srcdir) && ! $(XMLLINT) --noout $(XML_FILES) 2>&1 | grep '.'
check: xmllint-check

.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA += stamp-autochecks
CLEANFILES += stamp-autochecks
