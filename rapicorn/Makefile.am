# Rapicorn
# Copyright (C) 2005 Tim Janik

srcdir          = .
INCLUDES  	= -I$(top_srcdir) -I$(top_builddir) # -I$(srcdir)/cairo
GLIB_MKENUMS    = glib-mkenums
LDLIBS	       += $(LIBS)
DEFS		= -DRAPICORN_PRIVATE

rapicorn_public_headers = $(strip 	\
	rapicorn.hh keycodes.hh		\
	birnetutils.hh			\
	birnetsignal.hh			\
	birnetsignalvariants.hh		\
	enumdefs.hh			\
	primitives.hh			\
	appearance.hh			\
	events.hh			\
)
rapicorn_cc_sources = $(strip 		\
	birnetutils.cc			\
	enumdefs.cc			\
	primitives.cc			\
	appearance.cc			\
	events.cc			\
)
rapicorn_private_headers = $(strip 	\
	private.hh			\
	birnetsignaltemplate.hh		\
)
rapicorn_extra_sources = $(strip 	\
	mksignals.sh			\
)

lib_LTLIBRARIES        = librapicorn.la
librapicornincludedir  = $(includedir)/rapicorn
librapicorninclude_HEADERS  = $(rapicorn_public_headers)
librapicorn_la_SOURCES      = $(rapicorn_cc_sources)
librapicorn_la_LIBADD  = $(strip 		\
	$(RAPICORN_LIBS) -lm 			\
) # -lfreetype -lfontconfig $(top_builddir)/cairo/libcairo.a  	$(top_builddir)/pixman/libpixman.a 	
librapicorn_la_LDFLAGS = $(strip 				\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) 	\
	-release $(LT_RELEASE) 					\
	-export-dynamic -no-undefined 				\
) # set libtool version and export symbols for resolving
EXTRA_DIST            += $(rapicorn_private_headers) $(rapicorn_extra_sources)

XML_FILES = $(strip standard.xml )

# xml linting
stamp-xmllint: $(XML_FILES)
	cd $(srcdir) \
	&& $(XMLLINT) --noout $(XML_FILES)
	touch $@
$(lib_LTLIBRARIES): stamp-xmllint

birnetutils.cc: birnetutils.hh birnetsignal.hh birnetsignalvariants.hh
birnetsignalvariants.hh: birnetsignaltemplate.hh $(srcdir)/mksignals.sh
	cd . \
	&& echo "/* Signal Variants -- Generated file, don't edit. (make $@ from $<) */" > xgen-sigs \
	&& echo >> xgen-sigs \
	&& $(srcdir)/mksignals.sh $< >>xgen-sigs \
	&& cp xgen-sigs $@ \
	&& rm -f xgen-sigs
CLEANFILES += xgen-sigs birnetsignalvariants.hh xgen-signals.sed

enumdefs.cc: gen-enums.cc
gen-enums.cc: enumdefs.hh Makefile
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
