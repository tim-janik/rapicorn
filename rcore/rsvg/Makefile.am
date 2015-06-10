# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE -D__RAPICORN_BUILD__ -DRSVG_COMPILATION
AM_CFLAGS   += $(RAPICORN_CORE_CFLAGS) $(RAPICORN_RSVG_CFLAGS) -Wno-missing-prototypes -Wno-missing-declarations
AM_CXXFLAGS += $(RAPICORN_CORE_CFLAGS) $(RAPICORN_RSVG_CFLAGS)
GLIB_MKENUMS = glib-mkenums

rcore_rsvg_public_headers = $(strip	\
)
rcore_rsvg_sources = $(sort		\
	librsvg-features.c rsvg-css.c rsvg-css.h rsvg-defs.c rsvg-defs.h rsvg-image.c rsvg-image.h \
	rsvg-io.c rsvg-io.h rsvg-paint-server.c rsvg-paint-server.h rsvg-path.c rsvg-path.h rsvg-private.h \
	rsvg-base-file-util.c rsvg-filter.c rsvg-filter.h rsvg-marker.c rsvg-marker.h rsvg-mask.c rsvg-mask.h \
	rsvg-shapes.c rsvg-shapes.h rsvg-size-callback.c rsvg-size-callback.h \
	rsvg-structure.c rsvg-structure.h rsvg-styles.c rsvg-styles.h rsvg-text.c rsvg-text.h \
	rsvg-cairo-draw.c rsvg-cairo-draw.h rsvg-cairo-render.c rsvg-cairo-render.h rsvg-cairo-clip.h rsvg-cairo-clip.c \
	rsvg-cond.c rsvg-base.c rsvg.c rsvg-gobject.c rsvg-file-util.c rsvg-xml.c rsvg-xml.h \
	svg.hh svg.cc librsvg-enum-types.c \
)
rcore_rsvg_headers = $(sort rsvg.h rsvg-cairo.h )
rcore_rsvg_generated_headers = $(strip	\
)
rcore_rsvg_private_files = $(strip	\
	config.h			\
	librsvg-features.h		\
)

# === libraries ===
# librapicorn-core
noinst_LTLIBRARIES = librcore_rsvg@RAPICORN_RELEASE@.la # hardcoded -release $(RAPICORN_RELEASE)
librcore_rsvg@RAPICORN_RELEASE@_la_SOURCES      = $(rcore_rsvg_sources)
librcore_rsvg@RAPICORN_RELEASE@_la_DEPENDENCIES =
librcore_rsvg@RAPICORN_RELEASE@_la_LIBADD       = $(RAPICORN_RSVG_LIBS) $(RAPICORN_CORE_LIBS) -lm
librcore_rsvg@RAPICORN_RELEASE@_la_LDFLAGS      = $(strip	\
	$(AM_LDFLAGS) -no-undefined				\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) 	\
) # set libtool version and export symbols for resolving
EXTRA_DIST += $(rcore_rsvg_private_files) $(rcore_rsvg_headers)

# === API content ===
api-content:
	cat $(sort $(filter-out rapicornconfig.h, $(rapicorn_public_headers) $(rapicorn_generated_headers)))
.PHONY: api-content

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

librsvg-enum-types.h: $(rcore_rsvg_headers) Makefile
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) \
		--fhead "#ifndef __LIBRSVG_ENUM_TYPES_H__\n#define __LIBRSVG_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define RSVG_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n"  \
		--ftail "G_END_DECLS\n\n#endif /* __LIBRSVG_ENUM_TYPES_H__ */" \
	  $(rcore_rsvg_headers) ) > tmp-librsvg-enum-types.h \
	&& cp tmp-librsvg-enum-types.h librsvg-enum-types.h \
	&& rm -f tmp-librsvg-enum-types.h
librsvg-enum-types.c: librsvg-enum-types.h $(rcore_rsvg_headers) Makefile
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) \
		--fhead "#include \"librsvg-enum-types.h\"\n#include \"rsvg.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {"     \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
	  $(rcore_rsvg_headers) ) > tmp-librsvg-enum-types.c \
	&& cp tmp-librsvg-enum-types.c librsvg-enum-types.c \
	&& rm -f tmp-librsvg-enum-types.c
