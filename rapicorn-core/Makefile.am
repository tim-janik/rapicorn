# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS		 = . tests

INCLUDES	+= -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(RAPICORN_DEP_CFLAGS)
DEFS		 = -DRAPICORN_INTERNALS -DRAPICORN_LOG_DOMAIN=\"RAPICORN\" # "
GLIB_MKENUMS	 = glib-mkenums

rapicorn_top_headers = rapicorn-core.hh
rapicorn_public_headers = $(strip	\
        rapicorncpu.hh	rapicorndebugtools.hh	rapicornthread.hh	rapicornutils.hh  \
        rapicornmsg.hh	rapicornsignal.hh	rapicornmath.hh		rapicornutf8.hh   \
        rapicornxml.hh	markup.hh		enumdefs.hh		testutils.hh \
	$(rapicorn_impl_headers)	\
	rapicorncdefs.h	rapicorntests.h	\
)
rapicorn_impl_headers = $(strip 	\
)
rapicorn_generated_headers = $(strip	\
        rapicornsignalvariants.hh	\
	rapicornconfig.h		\
)
rapicorn_cc_sources = $(strip 		\
        rapicornthreadimpl.cc		\
        rapicorncpu.cc    rapicorndebugtools.cc	rapicornthread.cc	rapicornutils.cc  \
        rapicornmsg.cc    rapicornsignal.cc	rapicornmath.cc		rapicornutf8.cc   \
        rapicornxml.cc    markup.cc		enumdefs.cc		testutils.cc \
)
rapicorn_private_files = $(strip	\
        rapicornsignalslot.hh		\
        rapicornsignaltemplate.hh	\
	mksignals.sh			\
)

# === CPU flags ===
AM_CXXFLAGS = @FPU_FLAGS@

# === libraries ===
topincludedir   = $(includedir)/rapicorn@LT_RELEASE@
topinclude_DATA = $(rapicorn_top_headers)
EXTRA_DIST     += $(rapicorn_top_headers)
# librapicorn-core
noinst_LTLIBRARIES			    = librapicorncore@LT_RELEASE@.la # hardcoded -release $(LT_RELEASE)
librapicorncore@LT_RELEASE@includedir       = $(includedir)/rapicorn@LT_RELEASE@/rapicorn-core
librapicorncore@LT_RELEASE@include_HEADERS  = $(rapicorn_public_headers) $(rapicorn_generated_headers)
librapicorncore@LT_RELEASE@_la_SOURCES      = $(rapicorn_cc_sources)
librapicorncore@LT_RELEASE@_la_DEPENDENCIES = ldscript.map
librapicorncore@LT_RELEASE@_la_LIBADD       = $(RAPICORN_DEP_LIBS) -lm
librapicorncore@LT_RELEASE@_la_LDFLAGS      = $(strip		\
	-Wl,--version-script=$(srcdir)/ldscript.map		\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) 	\
	-no-undefined 						\
) # set libtool version and export symbols for resolving
EXTRA_DIST += $(rapicorn_private_files) $(librapicorncore@LT_RELEASE@_la_DEPENDENCIES)

# keep this .o rule in sync with the corresponding .la rule from Makefile.in
librapicorncore@LT_RELEASE@.o: $(librapicorncore@LT_RELEASE@_la_OBJECTS) $(librapicorncore@LT_RELEASE@_la_DEPENDENCIES)
	$(CXXLINK) -rpath $(libdir) $(librapicorncore@LT_RELEASE@_la_LDFLAGS) $(librapicorncore@LT_RELEASE@_la_OBJECTS) # $(librapicorncore@LT_RELEASE@_la_LIBADD) $(LIBS)
all-am: librapicorncore@LT_RELEASE@.o
CLEANFILES += librapicorncore@LT_RELEASE@.lo librapicorncore@LT_RELEASE@.o

# === API content ===
api-content:
	cat $(sort $(filter-out rapicornconfig.h, $(rapicorn_public_headers) $(rapicorn_top_headers) $(rapicorn_generated_headers)))
.PHONY: api-content

# === programs (not installed) ===
noinst_PROGRAMS        = $(ALLTESTS)
progs_ldadd            = librapicorncore@LT_RELEASE@.la $(RAPICORN_DEP_LIBS) -lm
# rapicorn-zintern
noinst_PROGRAMS       += rapicorn-zintern
rapicorn_zintern_SOURCES = rapicorn-zintern.cc
rapicorn_zintern_LDADD   = $(RAPICORN_DEP_LIBS) -lm # $(progs_ldadd)


# === enum introspection ===
$(srcdir)/enumdefs.cc: gen-enums.cc
gen-enums.cc: $(srcdir)/enumdefs.hh Makefile
	cd . \
	&& ( cd ${srcdir} && $(GLIB_MKENUMS) \
	  --fprod "\n/* --- @filename@ --- */\n#include\t\"@filename@\"" \
	  --vhead "/* @EnumName@\n */\n" \
	  --vhead "static const @Type@Type<@EnumName@>::Value @EnumName@_value_array[] = {" \
	  --vprod "  { @VALUENAME@, \"@VALUENAME@\", sizeof (\"@VALUENAME@\") - 1 }," \
	  --vtail "};\n" \
	  --vtail "template<> const uint @Type@Type<@EnumName@>::n_values = sizeof (@EnumName@_value_array) / sizeof (@EnumName@_value_array[0]);\n" \
	  --vtail "template<> const @Type@Type@EnumName@::Value *const @Type@Type@EnumName@::values = @EnumName@_value_array;\n" \
	  --vtail "template<> const char *@Type@Type@EnumName@::ename = \"@EnumName@\";" \
	    $^ ) > xgen-enu \
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
	&& echo "#define RAPICORN_CHECK_VERSION(majv,relv,revv)	\\"				>> xgen-$(@F) \
	&& echo "  (@RAPICORN_RELEASE_YEAR@ > (majv) || \\"					>> xgen-$(@F) \
	&& echo "    (@RAPICORN_RELEASE_YEAR@ == (majv) && \\"					>> xgen-$(@F) \
	&& echo "      (@RAPICORN_RELEASE_MONTH@ > (relv) || \\"					>> xgen-$(@F) \
	&& echo "        (@RAPICORN_RELEASE_MONTH@ == (relv) && \\"				>> xgen-$(@F) \
	&& echo "          (@RAPICORN_REVISION_VERSION@ >= (revv))))))"				>> xgen-$(@F) \
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
	&& echo "/* --- backend configuration --- */"						>> xgen-$(@F) \
	&& echo "#define RAPICORN_WITH_GTK	(1)"						>> xgen-$(@F) \
	&& echo "#define RAPICORN_WITH_PANGO	(1)"						>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- configure results --- */"						>> xgen-$(@F) \
	&& echo "#define RAPICORN_OS_@RAPICORN_OS@  	(1) /* one of: @RAPICORN_OS_CHOICES@ */"	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_POINTER		(@RAPICORN_SIZEOF_POINTER@)"		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_LONG		(@RAPICORN_SIZEOF_LONG@)"			>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_SYS_TYPESH_UINT	(@RAPICORN_SIZEOF_SYS_TYPESH_UINT@)"	>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_PTH_MUTEX_T	(@RAPICORN_SIZEOF_PTH_MUTEX_T@)" 		>> xgen-$(@F) \
	&& echo "#define RAPICORN_SIZEOF_PTH_COND_T	(@RAPICORN_SIZEOF_PTH_COND_T@)" 		>> xgen-$(@F) \
	&& echo "#define RAPICORN_HAVE_MUTEXATTR_SETTYPE	(@RAPICORN_HAVE_MUTEXATTR_SETTYPE@ && \\" >> xgen-$(@F) \
	&& echo "                                        RAPICORN_SIZEOF_PTH_MUTEX_T && \\" 	>> xgen-$(@F) \
	&& echo "                                        RAPICORN_SIZEOF_PTH_COND_T)" 		>> xgen-$(@F) \
	&& echo "#define RAPICORN_MEMORY_BARRIER_NEEDED   (@RAPICORN_MEMORY_BARRIER_NEEDED@)"	>> xgen-$(@F) \
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
$(librapicorncore@LT_RELEASE@_la_OBJECTS): rapicornconfig.h


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
