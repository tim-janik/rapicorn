# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS	     = . rsvg tests

INCLUDES    += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(RAPICORN_CORE_CFLAGS)
DEFS	     = -DRAPICORN_CONVENIENCE -D__SOURCE_COMPONENT__='"Rapicorn@RAPICORN_RELEASE@/$(subdir)/$<"' \
	       -D__RAPICORN_BUILD__
PLICINT	     = $(top_builddir)/plic/plic-intern
GLIB_MKENUMS = glib-mkenums

rapicorn_public_headers = $(strip	\
        rapicorncpu.hh	rapicorndebugtools.hh				utilities.hh  \
        rapicornmsg.hh	rapicornsignal.hh	rapicornmath.hh		rapicornutf8.hh   \
        rapicornxml.hh	markup.hh		enumdefs.hh		testutils.hh \
        pixmap.hh	regex.hh  strings.hh	loop.hh		main.hh	quicktimer.hh \
	thread.hh	\
	$(rapicorn_impl_headers) \
	rapicorncdefs.h	rcore.hh \
)
rapicorn_cc_sources = $(sort	serverglue.cc   clientglue.cc	\
        rapicorncpu.cc  rapicorndebugtools.cc				utilities.cc  \
        rapicornmsg.cc  rapicornsignal.cc	rapicornmath.cc		rapicornutf8.cc   \
        rapicornxml.cc  markup.cc		enumdefs.cc		testutils.cc \
			regex.cc  strings.cc	loop.cc		main.cc	quicktimer.cc \
	thread.cc	\
        plicwrapper.cc	\
)
rapicorn_impl_headers = $(strip 	\
	threadlib.hh			\
)
rapicorn_generated_headers = $(strip	\
        rapicornsignalvariants.hh	\
	rapicornconfig.h		\
	clientapi.hh			\
)
rapicorn_private_files = $(strip	\
        rapicornsignalslot.hh		\
        rapicornsignaltemplate.hh	\
        configbits.cc			\
	mksignals.sh			\
	pixmap.cc			\
)

# === CPU flags ===
AM_CXXFLAGS = @FPU_FLAGS@

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
) # set libtool version and export symbols for resolving
EXTRA_DIST += $(rapicorn_private_files) $(librapicorncore@RAPICORN_RELEASE@_la_DEPENDENCIES)

# == Optimization Targets ==
OPTIMIZE_SOURCE_FILES = loop.cc
CXXFLAGS += $(patsubst %, -O3 -minline-all-stringops, $(findstring $(<F), $(OPTIMIZE_SOURCE_FILES)))

# keep this .o rule in sync with the corresponding .la rule from Makefile.in
librapicorncore@RAPICORN_RELEASE@.o: $(librapicorncore@RAPICORN_RELEASE@_la_OBJECTS) $(librapicorncore@RAPICORN_RELEASE@_la_DEPENDENCIES)
	$(CXXLINK) -rpath $(libdir) $(librapicorncore@RAPICORN_RELEASE@_la_LDFLAGS) $(librapicorncore@RAPICORN_RELEASE@_la_OBJECTS) # $(librapicorncore@RAPICORN_RELEASE@_la_LIBADD) $(LIBS)
all-am: librapicorncore@RAPICORN_RELEASE@.o
CLEANFILES += librapicorncore@RAPICORN_RELEASE@.lo librapicorncore@RAPICORN_RELEASE@.o

# === API content ===
api-content:
	cat $(sort $(filter-out rapicornconfig.h, $(rapicorn_public_headers) $(rapicorn_generated_headers)))
.PHONY: api-content

# === programs (not installed) ===
noinst_PROGRAMS        = $(ALLTESTS)
progs_ldadd            = librapicorncore@RAPICORN_RELEASE@.la $(RAPICORN_CORE_LIBS) -lm
# rapicorn-zintern
noinst_PROGRAMS       += rapicorn-zintern
rapicorn_zintern_SOURCES = rapicorn-zintern.cc
rapicorn_zintern_LDADD   = $(RAPICORN_CORE_LIBS) -lm # $(progs_ldadd)

# === enum introspection ===
$(srcdir)/enumdefs.cc: gen-enums.cc
gen-enums.cc: $(srcdir)/enumdefs.hh Makefile
	cd . \
	&& ( cd ${srcdir} && $(GLIB_MKENUMS) \
	  --fprod "\n/* --- $< --- */\n#include\t\"$<\"" \
	  --vhead "/* @EnumName@\n */\n" \
	  --vhead "static const @Type@Type<@EnumName@>::Value @EnumName@_value_array[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", sizeof (\"@VALUENAME@\") - 1 }," \
	  --vtail "};\n" \
	  --vtail "template<> const uint @Type@Type<@EnumName@>::n_values = sizeof (@EnumName@_value_array) / sizeof (@EnumName@_value_array[0]);\n" \
	  --vtail "template<> const @Type@Type@EnumName@::Value *const @Type@Type@EnumName@::values = @EnumName@_value_array;\n" \
	  --vtail "template<> const char *@Type@Type@EnumName@::ename = \"@EnumName@\";" \
	    <(sed 's,//[/!].*,,' $<) ) > xgen-enu \
	&& cp xgen-enu $@ \
	&& rm -f xgen-enu
CLEANFILES += xgen-enu gen-enums.cc


# === configbits.cc ===
BUILDID=$(shell test -x $(top_srcdir)/.git/ && ( \
	git log -n1 --pretty=format:Git-%h --abbrev=11 ; \
	C=`git diff HEAD --raw | wc -l ` ; test "$$C" -lt 1 || echo "+$$C" ) \
	|| sed -n "1,1{s/.*\# */ChangeLog-/g;p}" <$(top_srcdir)/ChangeLog )
BUILDID_DEPS=$(shell B=`git rev-parse --git-dir 2>/dev/null` && for r in "`git symbolic-ref -q HEAD || echo HEAD`" packed-refs HEAD ; do echo "$B/$r" ; done)
configbits.cc: $(top_builddir)/config.status $(BUILDID_DEPS) PHONY_REBUILDS # Makefile
	$(Q) cd . \
	&& echo "// Generated data from $< (by make $@)" 					 > xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo '#define RAPICORN_BUILDID	"$(BUILDID)"'					>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "// Generated data ends here"	 						>> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) $@ || cp xgen-$(@F) $@)
	$(Q) rm -f xgen-$(@F)
CLEANFILES += xgen-configbits.cc configbits.cc
$(librapicorncore@RAPICORN_RELEASE@_la_OBJECTS): configbits.cc
.PHONY: PHONY_REBUILDS

# === Rapicorn Basics IDL ===
PLICINT     = $(top_builddir)/plic/plic-intern
CXXSTUB_PY  = $(top_srcdir)/plic/CxxStub.py
EXTRA_DIST += basics.idl serverinsert.hh
# build serverapi.{hh|cc} and clientapi.{hh|cc} from basics.idl
$(rapicorn_cc_sources): serverapi.hh clientapi.hh
serverapi.hh: $(CXXSTUB_PY) $(srcdir)/basics.idl $(srcdir)/serverinsert.hh
	${PLICINT} -G CxxStub -g serverhh -g shortalias -g cppguard=__RAPICORN_RCORE_SERVERAPI_HH_ \
	  --insertions $(srcdir)/serverinsert.hh $(srcdir)/basics.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
serverglue.cc: serverapi.cc
serverapi.cc: $(CXXSTUB_PY) $(srcdir)/basics.idl serverapi.hh
	${PLICINT} -G CxxStub -g servercc --inclusions '"serverapi.hh"' $(srcdir)/basics.idl -o - >> xgen-$(@F)
	sed '/#include <plic\/cxxstubaux.hh>/s,^,// ,' -i xgen-$(@F)
	mv xgen-$(@F) $@
CLEANFILES += serverapi.cc serverapi.hh
EXTRA_DIST += clientinsert.hh
clientapi.hh: $(CXXSTUB_PY) $(srcdir)/basics.idl $(srcdir)/clientinsert.hh
	${PLICINT} -G CxxStub -g clienthh -g shortalias -g cppguard=__RAPICORN_RCORE_CLIENTAPI_HH_ \
	  --insertions $(srcdir)/clientinsert.hh $(srcdir)/basics.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
clientglue.cc: clientapi.cc
clientapi.cc: $(CXXSTUB_PY) $(srcdir)/basics.idl clientapi.hh
	${PLICINT} -G CxxStub -g clientcc --inclusions '"clientapi.hh"' $(srcdir)/basics.idl -o - >> xgen-$(@F)
	sed '/#include <plic\/cxxstubaux.hh>/s,^,// ,' -i xgen-$(@F)
	mv xgen-$(@F) $@
CLEANFILES += clientapi.cc clientapi.hh

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
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- standard C code wrappers --- */"					>> xgen-$(@F) \
	&& echo "#ifdef  __cplusplus"								>> xgen-$(@F) \
	&& echo '#define RAPICORN_EXTERN_C_BEGIN()	extern "C" {'				>> xgen-$(@F) \
	&& echo "#define RAPICORN_EXTERN_C_END()		}"					>> xgen-$(@F) \
	&& echo "#else"										>> xgen-$(@F) \
	&& echo "#define RAPICORN_EXTERN_C_BEGIN()"						>> xgen-$(@F) \
	&& echo "#define RAPICORN_EXTERN_C_END()"							>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- rapicorn version --- */"						>> xgen-$(@F) \
	&& echo '#define RAPICORN_VERSION  "@RAPICORN_RELEASE_YEAR@.@RAPICORN_RELEASE_MONTH@.@RAPICORN_REVISION_VERSION@"' >> xgen-$(@F) \
	&& echo "/* version checks */"								>> xgen-$(@F) \
	&& echo "#define RAPICORN_CHECK_VERSION(year,month,revv) \\"				>> xgen-$(@F) \
	&& echo "  (@RAPICORN_RELEASE_YEAR@ > (year) || \\"					>> xgen-$(@F) \
	&& echo "    (@RAPICORN_RELEASE_YEAR@ == (year) && \\"					>> xgen-$(@F) \
	&& echo "      (13@RAPICORN_RELEASE_MONTH@%13 > (month) || \\"				>> xgen-$(@F) \
	&& echo "        (13@RAPICORN_RELEASE_MONTH@%13 == (month) && \\"				>> xgen-$(@F) \
	&& echo "          (@RAPICORN_REVISION_VERSION@ >= (revv))))))"				>> xgen-$(@F) \
	&& echo '#define RAPICORN_DEVEL_VERSION  (1 & @RAPICORN_REVISION_VERSION@)' 		>> xgen-$(@F) \
	&& echo '#define RAPICORN_STABLE_VERSION (!RAPICORN_DEVEL_VERSION)' 			>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- log & i18n domains --- */"						>> xgen-$(@F) \
	&& echo "#ifndef RAPICORN_LOG_DOMAIN"							>> xgen-$(@F) \
	&& echo "#ifdef  G_LOG_DOMAIN"								>> xgen-$(@F) \
	&& echo "#define RAPICORN_LOG_DOMAIN  G_LOG_DOMAIN"					>> xgen-$(@F) \
	&& echo "#else"										>> xgen-$(@F) \
	&& echo "#define RAPICORN_LOG_DOMAIN  ((const char*) 0)"				>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
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
	&& echo "#define RAPICORN_WITH_GTK	(1)"						>> xgen-$(@F) \
	&& echo "#define RAPICORN_WITH_PANGO	(1)"						>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- configure results --- */"						>> xgen-$(@F) \
	&& echo "#define RAPICORN_OS_@RAPICORN_OS@  	(1) /* one of: @RAPICORN_OS_CHOICES@ */"	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_POINTER		(@RAPICORN_SIZEOF_POINTER@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_LONG		(@RAPICORN_SIZEOF_LONG@)"			>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_SYS_TYPESH_UINT	(@RAPICORN_SIZEOF_SYS_TYPESH_UINT@)"	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_PTHREADH_SPINLOCK	(@RAPICORN_SIZEOF_PTHREADH_SPINLOCK@)" 		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_PTH_MUTEX_T	(@RAPICORN_SIZEOF_PTH_MUTEX_T@)" 		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_PTH_COND_T	(@RAPICORN_SIZEOF_PTH_COND_T@)" 		>> xgen-$(@F) \
	&& echo "#define RAPICORN_HAVE_MUTEXATTR_SETTYPE	(@RAPICORN_HAVE_MUTEXATTR_SETTYPE@ && \\" >> xgen-$(@F) \
	&& echo "                                        RAPICORN_SIZEOF_PTH_MUTEX_T && \\" 	>> xgen-$(@F) \
	&& echo "                                        RAPICORN_SIZEOF_PTH_COND_T)" 		>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLIN 		(@RAPICORN_SYSVAL_POLLIN@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLPRI		(@RAPICORN_SYSVAL_POLLPRI@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLOUT		(@RAPICORN_SYSVAL_POLLOUT@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLRDNORM	(@RAPICORN_SYSVAL_POLLRDNORM@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLRDBAND 	(@RAPICORN_SYSVAL_POLLRDBAND@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLWRNORM   	(@RAPICORN_SYSVAL_POLLWRNORM@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLWRBAND	(@RAPICORN_SYSVAL_POLLWRBAND@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLERR		(@RAPICORN_SYSVAL_POLLERR@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLHUP		(@RAPICORN_SYSVAL_POLLHUP@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SYSVAL_POLLNVAL		(@RAPICORN_SYSVAL_POLLNVAL@)"		>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* Generated data ends here */" 						>> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
CLEANFILES += xgen-rapicornconfig.h rapicornconfig.h
$(librapicorncore@RAPICORN_RELEASE@_la_OBJECTS): rapicornconfig.h


# === rapicornsignalvariants.hh ===
$(srcdir)/*.cc: rapicornsignalvariants.hh
rapicornsignalvariants.hh: $(srcdir)/rapicornsignalslot.hh $(srcdir)/rapicornsignaltemplate.hh $(srcdir)/mksignals.sh
	cd . \
	&& echo "/* Signal Variants -- Generated from: $(^F) */"	>xgen-sigs \
	&& echo 						       >>xgen-sigs \
	&& $(srcdir)/mksignals.sh $(srcdir)/rapicornsignalslot.hh     17 >>xgen-sigs \
	&& $(srcdir)/mksignals.sh $(srcdir)/rapicornsignaltemplate.hh 16 >>xgen-sigs \
	&& cp xgen-sigs $@ \
	&& rm -f xgen-sigs
CLEANFILES += xgen-sigs rapicornsignalvariants.hh xgen-signals.sed
