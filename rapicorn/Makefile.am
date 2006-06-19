# Rapicorn
# Copyright (C) 2005 Tim Janik

INCLUDES  	= -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(PANGO_CFLAGS) $(GTK_CFLAGS) -I$(top_builddir)/cairo-inlined
GLIB_MKENUMS    = glib-mkenums
LDLIBS	       += $(LIBS) $(PANGO_LIBS) $(GTK_LIBS)
DEFS		= -DRAPICORN_PRIVATE -DPARANOID

rapicorn_public_headers = $(strip 	\
	rapicorn.hh	keycodes.hh	userconfig.hh	handle.hh	\
	$(rapicorn_impl_headers)	\
	enumdefs.hh	viewport.hh	\
	arrangement.hh	primitives.hh	birnetutils.hh	birnetmarkup.hh	utilities.hh	paintcontainers.hh	\
	appearance.hh	events.hh	item.hh		container.hh	factory.hh	layoutcontainers.hh	\
	paintgadgets.hh	slider.hh	buttons.hh	image.hh	\
	table.hh	pangolabel.hh	loop.hh		root.hh		\
	commands.hh	properties.hh	adjustment.hh	painter.hh	cairocontext.hh	\
)
rapicorn_impl_headers = $(strip 	\
	arrangementimpl.hh		\
	containerimpl.hh		\
	itemimpl.hh			\
	rootimpl.hh			\
	tableimpl.hh			\
)
rapicorn_cc_sources = $(strip 		\
	viewport-gtk.cc			\
	$(strip )			\
	enumdefs.cc	viewport.cc	\
	arrangement.cc	primitives.cc	birnetutils.cc	birnetmarkup.cc	utilities.cc	paintcontainers.cc	\
	appearance.cc	events.cc	item.cc		container.cc	factory.cc	layoutcontainers.cc	\
	paintgadgets.cc	slider.cc	buttons.cc	image.cc	\
	table.cc	pangolabel.cc	loop.cc		root.cc 	\
	commands.cc	properties.cc	adjustment.cc	painter.cc	cairocontext.cc	\
)
rapicorn_private_headers = $(strip 	\
	private.hh			\
)
rapicorn_fastmath_files = $(strip	\
)
CXXFLAGS += $(if $(filter $(rapicorn_fastmath_files), $<), -ffast-math)

lib_LTLIBRARIES            = librapicorn.la
librapicornincludedir      = $(includedir)/rapicorn
librapicorninclude_HEADERS = $(rapicorn_public_headers)
librapicorn_la_SOURCES     = $(rapicorn_cc_sources)
librapicorn_la_DEPENDENCIES += ldscript.map
librapicorn_la_LDFLAGS = $(strip 				\
	-Wl,--version-script=$(srcdir)/ldscript.map		\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) 	\
	-release $(LT_RELEASE) 					\
	-no-undefined 						\
) # set libtool version and export symbols for resolving
#
librapicorn_la_LIBADD      = $(strip 			\
	$(top_builddir)/cairo-inlined/libpixman.la	\
	$(top_builddir)/cairo-inlined/libcairo.la	\
	$(top_builddir)/birnet/libbirnet.o		\
	$(RAPICORN_LIBS) -lm 				\
) # -lfreetype -lfontconfig
#
EXTRA_DIST += $(rapicorn_private_headers)

#XML_FILES = $(strip standard.xml )
## xml linting
#stamp-xmllint: $(XML_FILES)
#	cd $(srcdir) \
#	&& $(XMLLINT) --noout $(XML_FILES)
#	touch $@
#$(lib_LTLIBRARIES): stamp-xmllint

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
