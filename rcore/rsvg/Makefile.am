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
noinst_LTLIBRARIES                       = librapicorn-rsvg-@MAJOR@.la
librapicorn_rsvg_@MAJOR@_la_SOURCES      = $(rcore_rsvg_sources)
librapicorn_rsvg_@MAJOR@_la_DEPENDENCIES =
librapicorn_rsvg_@MAJOR@_la_LIBADD       = $(RAPICORN_RSVG_LIBS) $(RAPICORN_CORE_LIBS) -lm
librapicorn_rsvg_@MAJOR@_la_LDFLAGS      = $(strip	\
	$(AM_LDFLAGS) -no-undefined			\
	-version-info $(LT_VERSION_INFO) 		\
) # set libtool version and export symbols for resolving
EXTRA_DIST += $(rcore_rsvg_private_files) $(rcore_rsvg_headers)

# === enum introspection ===
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
$(librapicorn_rsvg_@MAJOR@_la_SOURCES:%=$(srcdir)/%): librsvg-enum-types.h
EXTRA_DIST += librsvg-enum-types.h librsvg-enum-types.c
CLEANFILES += @INGIT@ librsvg-enum-types.h librsvg-enum-types.c
