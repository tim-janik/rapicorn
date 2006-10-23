# Rapicorn
# Copyright (C) 2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

INCLUDES       += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(PANGO_CFLAGS) $(GTK_CFLAGS)
GLIB_MKENUMS    = glib-mkenums
LDLIBS	        = $(LIBS) $(PANGO_LIBS) $(GTK_LIBS)
DEFS		= -DRAPICORN_PRIVATE -DPARANOID

rapicorn_public_headers = $(strip 	\
	rapicorn.hh	keycodes.hh	userconfig.hh	handle.hh	\
	$(rapicorn_impl_headers)	\
	birnetutils.hh	birnetmarkup.hh	utilities.hh	events.hh	\
	enumdefs.hh	primitives.hh	region.hh	loop.hh		viewport.hh		\
	properties.hh	commands.hh	appearance.hh	painter.hh	\
	item.hh		container.hh	root.hh		table.hh	arrangement.hh		\
	adjustment.hh	image.hh	paintgadgets.hh	slider.hh	paintcontainers.hh	\
	buttons.hh	factory.hh	evaluator.hh	scrollitems.hh	layoutcontainers.hh	\
	text-editor.hh	text-pango.hh	testitem.hh	\
)
rapicorn_impl_headers = $(strip 	\
	arrangementimpl.hh		\
	containerimpl.hh		\
	itemimpl.hh			\
	rootimpl.hh			\
	tableimpl.hh			\
)
rapicorn_cc_sources = $(strip 		\
	viewport-gtk.cc	regionimpl.c	\
	$(strip )			\
	birnetutils.cc	birnetmarkup.cc	utilities.cc	events.cc	\
	enumdefs.cc	primitives.cc	region.cc	loop.cc		viewport.cc		\
	properties.cc	commands.cc	appearance.cc	painter.cc	\
	item.cc		container.cc	root.cc		table.cc	arrangement.cc		\
	adjustment.cc	image.cc	paintgadgets.cc	slider.cc	paintcontainers.cc	\
	buttons.cc	factory.cc	evaluator.cc	scrollitems.cc	layoutcontainers.cc	\
	text-editor.cc	text-pango.cc	testitem.cc	\
)
rapicorn_private_headers = $(strip 	\
	regionimpl.h			\
	private.hh			\
)
rapicorn_fastmath_files = $(strip	\
)
librapicorn_la_CXXFLAGS = $(if $(filter $(rapicorn_fastmath_files), $<), -ffast-math)

lib_LTLIBRARIES             = librapicorn.la
librapicornincludedir       = $(includedir)/rapicorn
librapicorninclude_HEADERS  = $(rapicorn_public_headers)
librapicorn_la_SOURCES      = $(rapicorn_cc_sources)
librapicorn_la_DEPENDENCIES = ldscript.map
librapicorn_la_LDFLAGS      = $(strip 				\
	-Wl,--version-script=$(srcdir)/ldscript.map		\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) 	\
	-release $(LT_RELEASE) 					\
	-no-undefined 						\
) # set libtool version and export symbols for resolving
#
librapicorn_la_LIBADD      = $(strip 			\
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

XML_FILES   = rapicorn.xml
XMLLINTCALL = $(if $(XMLLINT),$(XMLLINT),true) # handle undefined XMLLINT
xml-lint:
	$(XMLLINTCALL) --noout $(XML_FILES)
.PHONY: xml-lint
stamp-allchecks: $(XML_FILES)
	@$(MAKE) xml-lint
	@touch $@
noinst_DATA = stamp-allchecks

zintern.c:	$(top_builddir)/birnet/birnet-zintern $(srcdir)/rapicorn.xml stamp-allchecks
	$(top_builddir)/birnet/birnet-zintern -b -z RAPICORN $(srcdir)/rapicorn.xml > xgen-$(@F)
	mv xgen-$(@F) $@
factory.cc: zintern.c
