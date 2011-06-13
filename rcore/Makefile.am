# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS	     = . rsvg tests

INCLUDES    += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(RAPICORN_CORE_CFLAGS)
DEFS	     = -DRAPICORN_CONVENIENCE -D__SOURCE_FILE__='"$(patsubst $(realpath $(abs_top_srcdir))/%,%, $(realpath $<))"' \
	       -D__RAPICORN_BUILD__
GLIB_MKENUMS = glib-mkenums
PLICINT	     = $(top_builddir)/plic/plic-intern

rapicorn_public_headers = $(strip	\
        rapicorncpu.hh	rapicorndebugtools.hh	rapicornthread.hh	rapicornutils.hh  \
        rapicornmsg.hh	rapicornsignal.hh	rapicornmath.hh		rapicornutf8.hh   \
        rapicornxml.hh	markup.hh		enumdefs.hh		testutils.hh \
        pixmap.hh	regex.hh		plicutils.hh \
	values.hh 	types.hh 		models.hh \
	$(rapicorn_impl_headers) \
	rapicorncdefs.h	rapicorntests.h		rcore.hh		\
)
rapicorn_cc_sources = $(sort		\
        rapicorncpu.cc  rapicorndebugtools.cc	rapicornthread.cc	rapicornutils.cc  \
        rapicornmsg.cc  rapicornsignal.cc	rapicornmath.cc		rapicornutf8.cc   \
        rapicornxml.cc  markup.cc		enumdefs.cc		testutils.cc \
        pixmap.cc	regex.cc		plicutils.cc \
	values.cc 	types.cc 		models.cc    \
        rapicornthreadimpl.cc		\
)
rapicorn_impl_headers = $(strip 	\
)
rapicorn_generated_headers = $(strip	\
        rapicornsignalvariants.hh	\
	rapicornconfig.h		\
)
rapicorn_private_files = $(strip	\
        rapicornsignalslot.hh		\
        rapicornsignaltemplate.hh	\
	mksignals.sh			\
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

# === types-ptpp.cc ===
types-ptpp.cc: $(PLICINT)
	:; $(PLICINT) --cc-type-package-parser > gen-$@
	mv gen-$@ $@
types.cc: types-ptpp.cc
CLEANFILES += types-ptpp.cc
check-ptpp-version: $(top_srcdir)/plic/PlicTypePackage.cc
	$(Q) cp types-ptpp.cc $(TSTOUT)
	$(Q) eval "$$TSTVERIFY" "'$(top_srcdir)/plic/PlicTypePackage.cc'"
check: # check-ptpp-version # sanity check the --cc-type-package-parser rule

# === types-zgen.c ===
EXTRA_DIST += types-std.idl
types-zgen.c:  ./rapicorn-zintern $(srcdir)/types-std.idl
	: # compile IDL definitions into binary data
	$(PLICINT) $(srcdir)/types-std.idl -G TypePackage -o ./types-std.tpg
	: # convert to .rodata and fixup standard types
	./rapicorn-zintern -b RAPICORNSTD_RODATA -n ./types-std.tpg | \
	  sed -e 's/12=Rapicorn_STD/12=Rapicorn*STD/'	\
	      -e 's/__i\.\.3=iNt/__i..3=int/'			\
	      -e 's/__d\.\.5=floAt/__d..5=float/'			\
	      -e 's/__s\.\.6=strIng/__s..6=string/'		\
	      -e 's/__d\.\.5=Array/__a..5=Array/'		> xgen-$(@F)
	: # validate standard type adaptions
	test `grep  -F	-e '12=Rapicorn*STD'	\
			-e '__i..3=int'		\
			-e '__d..5=float'	\
			-e '__s..6=string'	\
			-e '__a..5=Array'   xgen-$(@F) | wc -l` = 5
	rm -f ./types-std.tpg
	mv xgen-$(@F) $@
$(srcdir)/types.cc: types-zgen.c
CLEANFILES += xgen-types-zgen.c types-zgen.c


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
