# Rapicorn
include $(top_srcdir)/Makefile.decl

SUBDIRS	     = . rsvg tests

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE -D__RAPICORN_BUILD__
AM_CXXFLAGS += $(RAPICORN_CORE_CFLAGS)
GLIB_MKENUMS = glib-mkenums

rapicorn_public_headers = $(strip	\
	aida.hh		aidaprops.hh	\
	bindable.hh	resources.hh	formatter.hh	inout.hh	inifile.hh \
        platform.hh	debugtools.hh					utilities.hh  \
					math.hh				unicode.hh   \
	objects.hh	\
        xmlnode.hh	markup.hh					testutils.hh \
			regex.hh  strings.hh	loop.hh		main.hh	memory.hh \
	quicktimer.hh	randomhash.hh	thread.hh	\
	$(rapicorn_impl_headers) \
	cpuasm.hh	cxxaux.hh	rcore.hh	aidasignal.hh	\
)
rapicorn_cc_sources = $(sort	\
	aida.cc		aidaprops.cc	\
	bindable.cc	resources.cc	formatter.cc	inout.cc	inifile.cc \
        platform.cc	debugtools.cc					utilities.cc  \
					math.cc				unicode.cc   \
	objects.cc	\
        xmlnode.cc	markup.cc					testutils.cc \
			regex.cc  strings.cc	loop.cc		main.cc	memory.cc \
	quicktimer.cc	randomhash.cc	thread.cc	\
)
rapicorn_impl_headers = $(strip 	\
	aidacxx.hh			\
	threadlib.hh			\
)
rapicorn_generated_headers = $(strip	\
	aidavariants.hh			\
	rapicornconfig.h		\
)
rapicorn_private_files = $(strip	\
)
rapicorn_generated_files = $(strip    	\
	configbits.cc			\
	zres.cc				\
)
# rapicorn_generated_files are NOT in EXTRA_DIST

# === libraries ===
# librapicorn-core
noinst_LTLIBRARIES = librapicorncore@RAPICORN_RELEASE@.la # hardcoded -release $(RAPICORN_RELEASE)
librapicorncore@RAPICORN_RELEASE@includedir       = $(includedir)/rapicorn@RAPICORN_RELEASE@/rcore
librapicorncore@RAPICORN_RELEASE@include_HEADERS  = $(rapicorn_public_headers) $(rapicorn_generated_headers)
librapicorncore@RAPICORN_RELEASE@_la_SOURCES      = $(rapicorn_cc_sources)
librapicorncore@RAPICORN_RELEASE@_la_DEPENDENCIES = ldscript.map
librapicorncore@RAPICORN_RELEASE@_la_LIBADD       = $(RAPICORN_CORE_LIBS) -lm
librapicorncore@RAPICORN_RELEASE@_la_LDFLAGS      = $(strip	\
	-Wl,--version-script=$(srcdir)/ldscript.map		\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) 	\
	-no-undefined $(SYMBOLIC_LDFLAGS)			\
	$(AM_LDFLAGS)                                           \
) # set libtool version and export symbols for resolving
EXTRA_DIST += $(rapicorn_private_files) ldscript.map

# == Special Optimization Targets ==
OPTIMIZE_SOURCE_FILES = loop.cc randomhash.cc
AM_CXXFLAGS += $(patsubst %, @OPTIMIZE_FAST@, $(findstring $(<F), $(OPTIMIZE_SOURCE_FILES)))

# == Rapicorn Resources ==
@mk@ include $(top_srcdir)/res/Makefile.res	# define RES_FILE_LIST
zres.cc: rapicorn-zintern $(top_srcdir)/res/Makefile.res
	$(AM_V_GEN)
	$(Q) (cd $(top_srcdir)/res/ && $(abs_builddir)/rapicorn-zintern -r $(RES_FILE_LIST) ) > xgen-$@
	$(Q) mv xgen-$@ $@
CLEANFILES += xgen-zres.cc zres.cc
$(srcdir)/resources.cc: zres.cc

# === API content ===
api-content:
	cat $(sort $(filter-out rapicornconfig.h, $(rapicorn_public_headers) $(rapicorn_generated_headers)))
.PHONY: api-content

# === programs (not installed) ===
noinst_PROGRAMS        = $(ALLTESTS)
progs_ldadd            = librapicorncore@RAPICORN_RELEASE@.la $(RAPICORN_CORE_LIBS) -lm

# == Utilities (for installation) ==
bin_PROGRAMS      	 = rapicorn-zintern
rapicorn_zintern_SOURCES = rapicorn-zintern.cc
rapicorn_zintern_LDADD   = $(LIBZ) # $(progs_ldadd)

zintern-installcheck:
	$(Q) cd . \
	&& echo '0123456789abcdef 0123456789abcdef c61fd6dd8e5a 0123456789abcdef c61fd6dd8e5a' > xtmp-empty.dat \
	; eval "$$TSTDIAGNOSE" "'Create zintern sample data'"
	$(Q) $(bindir)/rapicorn-zintern -z -b xtmp-empty.dat > xtmp-empty.out \
	; eval "$$TSTDIAGNOSE" "'Run    rapicorn-zintern'"
	$(Q) grep -q '"xtmp-empty.dat"' xtmp-empty.out \
	; eval "$$TSTDIAGNOSE" "'Verify zintern output file name'"
	$(Q) test `grep _SIZE xtmp-empty.out | grep -o '[0-9]*'` -gt 16 \
	; eval "$$TSTDIAGNOSE" "'Verify zintern output length'"
	$(Q) test $$[`grep _DATA xtmp-empty.out | grep -o '[0-9 +-]*'`] -lt 50 \
	; eval "$$TSTDIAGNOSE" "'Verify zintern output compression'"
	$(Q) rm -f xtmp-empty.dat xtmp-empty.out
installcheck-local: zintern-installcheck
CLEANFILES += xtmp-empty.dat xtmp-empty.out

# === configbits.cc ===
BUILDID=$(shell test -x $(top_srcdir)/.git/ && ( \
	git log -n1 --pretty=format:Git-%h --abbrev=12 ; \
	C=`git diff HEAD --raw | wc -l ` ; test "$$C" -lt 1 || echo "+$$C" ) \
	|| sed -n "1,1{s/.*\# */ChangeLog-/g;p}" <$(top_srcdir)/ChangeLog )
BUILDID_DEPS=$(shell B=`git rev-parse --git-dir 2>/dev/null` && for r in "`git symbolic-ref -q HEAD || echo HEAD`" packed-refs HEAD ; do echo "$B/$r" ; done)
buildid:
	@echo $(BUILDID)
configbits.cc: $(top_builddir)/config.status $(BUILDID_DEPS) PHONY_REBUILDS # Makefile
	$(AM_V_GEN)
	$(Q) cd . \
	&& echo "// Generated data from $< (by make $@)" 					 > xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo '#define RAPICORN_BUILDID	"$(BUILDID)"'					>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "// Generated data ends here"	 						>> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $@ || cp xgen-$(@F) $@)
	$(Q) rm -f xgen-$(@F)
CLEANFILES += xgen-configbits.cc configbits.cc
$(srcdir)/main.cc: configbits.cc
.PHONY: PHONY_REBUILDS buildid

# === rapicornconfig.h ===
rapicornconfig.h: $(top_builddir)/config.status # Makefile
	cd . \
	&& echo "/* Generated data from $< (by make $@) */" 					 > xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- select programming environment --- */"					>> xgen-$(@F) \
	&& echo "#ifndef _GNU_SOURCE"								>> xgen-$(@F) \
	&& echo "#define _GNU_SOURCE"								>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- unique namespace for all symbols --- */"				>> xgen-$(@F) \
	&& echo "#define Rapicorn			@RAPICORN_NAMESPACE_NAME@"		>> xgen-$(@F) \
	&& echo '#define RAPICORN_NAMESPACE_NAME	"@RAPICORN_NAMESPACE_NAME@"'		>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- standard C code wrappers --- */"					>> xgen-$(@F) \
	&& echo "#ifdef  __cplusplus"								>> xgen-$(@F) \
	&& echo '#define RAPICORN_EXTERN_C_BEGIN()	extern "C" {'				>> xgen-$(@F) \
	&& echo "#define RAPICORN_EXTERN_C_END()		}"				>> xgen-$(@F) \
	&& echo "#else"										>> xgen-$(@F) \
	&& echo "#define RAPICORN_EXTERN_C_BEGIN()"						>> xgen-$(@F) \
	&& echo "#define RAPICORN_EXTERN_C_END()"						>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- rapicorn version --- */"						>> xgen-$(@F) \
	&& echo '#define RAPICORN_VERSION  "@RAPICORN_RELEASE_YEAR@.@RAPICORN_RELEASE_MONTH@.@RAPICORN_REVISION_VERSION@"' >> xgen-$(@F) \
	&& echo "/* version checks */"								>> xgen-$(@F) \
	&& echo "#define RAPICORN_CHECK_VERSION(year,month,revv) \\"				>> xgen-$(@F) \
	&& echo "  (@RAPICORN_RELEASE_YEAR@ > (year) || \\"					>> xgen-$(@F) \
	&& echo "    (@RAPICORN_RELEASE_YEAR@ == (year) && \\"					>> xgen-$(@F) \
	&& echo "      (13@RAPICORN_RELEASE_MONTH@%13 > (month) || \\"				>> xgen-$(@F) \
	&& echo "        (13@RAPICORN_RELEASE_MONTH@%13 == (month) && \\"			>> xgen-$(@F) \
	&& echo "          (@RAPICORN_REVISION_VERSION@ >= (revv))))))"				>> xgen-$(@F) \
	&& echo '#define RAPICORN_DEVEL_VERSION  (1 & @RAPICORN_REVISION_VERSION@)' 		>> xgen-$(@F) \
	&& echo '#define RAPICORN_STABLE_VERSION (!RAPICORN_DEVEL_VERSION)' 			>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- i18n domain --- */"							>> xgen-$(@F) \
	&& echo '#define RAPICORN_I18N_DOMAIN    "rapicorn"'					>> xgen-$(@F) \
	&& echo "/* make sure to call bindtextdomain (RAPICORN_I18N_DOMAIN, i18ndir_path); */"	>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- resource paths --- */"							>> xgen-$(@F) \
	&& echo	'#define RAPICORN_DOCDIR		"${docdir}"'				>> xgen-$(@F) \
	&& echo	'#define RAPICORN_SVGDIR		"${rapicornsvgdir}"'			>> xgen-$(@F) \
	&& echo	'#define RAPICORN_DATADIR       	"${rapicorndatadir}"'			>> xgen-$(@F) \
	&& echo	'#define RAPICORN_LOCALEDIR     	"${pkglocaledir}"'			>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- backend configuration --- */"						>> xgen-$(@F) \
	&& echo "#define RAPICORN_WITH_PANGO	(1)"						>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- configure results --- */"						>> xgen-$(@F) \
	&& echo "#define RAPICORN_OS_@RAPICORN_OS@  	(1) /* one of: @RAPICORN_OS_CHOICES@ */"	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_POINTER		(@RAPICORN_SIZEOF_POINTER@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_LONG		(@RAPICORN_SIZEOF_LONG@)"			>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_SYS_TYPESH_UINT	(@RAPICORN_SIZEOF_SYS_TYPESH_UINT@)"	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_PTHREADH_SPINLOCK	(@RAPICORN_SIZEOF_PTHREADH_SPINLOCK@)" 	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_PTH_MUTEX_T	(@RAPICORN_SIZEOF_PTH_MUTEX_T@)" 		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_PTH_COND_T	(@RAPICORN_SIZEOF_PTH_COND_T@)" 		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SPINLOCK_INITIALIZER	@RAPICORN_SPINLOCK_INITIALIZER@" 		>> xgen-$(@F) \
	&& echo "#define RAPICORN_HAVE_MUTEXATTR_SETTYPE	(@RAPICORN_HAVE_MUTEXATTR_SETTYPE@ && \\" >> xgen-$(@F) \
	&& echo "                                        RAPICORN_SIZEOF_PTH_MUTEX_T && \\" 	>> xgen-$(@F) \
	&& echo "                                        RAPICORN_SIZEOF_PTH_COND_T)" 		>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLIN 		(@RAPICORN_SYSVAL_POLLIN@)"	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLPRI		(@RAPICORN_SYSVAL_POLLPRI@)"	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLOUT		(@RAPICORN_SYSVAL_POLLOUT@)"	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLRDNORM	(@RAPICORN_SYSVAL_POLLRDNORM@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLRDBAND 	(@RAPICORN_SYSVAL_POLLRDBAND@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLWRNORM   	(@RAPICORN_SYSVAL_POLLWRNORM@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLWRBAND	(@RAPICORN_SYSVAL_POLLWRBAND@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLERR		(@RAPICORN_SYSVAL_POLLERR@)"	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLHUP		(@RAPICORN_SYSVAL_POLLHUP@)"	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLNVAL	(@RAPICORN_SYSVAL_POLLNVAL@)"		>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* Generated data ends here */" 						>> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
CLEANFILES += xgen-rapicornconfig.h rapicornconfig.h
$(librapicorncore@RAPICORN_RELEASE@_la_OBJECTS): rapicornconfig.h


# === aidavariants.hh ===
$(srcdir)/*.cc: aidavariants.hh
aidavariants.hh: $(srcdir)/aidaproto.hh $(srcdir)/xmanifold.py
	$(Q) cd . \
	&& echo "/* Aida Variants -- Generated from: $(^F) */"	 >xtmp-$(@F) \
	&& echo '# 1 "aidaproto.hh"'				>>xtmp-$(@F) \
	&& $(srcdir)/xmanifold.py $(srcdir)/aidaproto.hh 18	>>xtmp-$(@F) \
	&& cp xtmp-$(@F) $@ && rm -f xtmp-$(@F)
CLEANFILES += xtmp-aidavariants.hh aidavariants.hh xgen-aidas.sed
EXTRA_DIST += aidaproto.hh xmanifold.py
# not using "$(abs_srcdir)/aidaproto.hh" to avoid hardcoded paths in distribution tarballs
