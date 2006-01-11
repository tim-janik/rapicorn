
INCLUDES = -I$(srcdir) -I$(top_builddir)/cairo-inlined $(PANGO_CFLAGS) -DUSER_LABEL_PREFIX=__RAPICRON_CAIRO_
CFLAGS += -Wno-redundant-decls -Wnested-externs -fno-strict-aliasing -Wpointer-arith -fPIC -DPIC

noinst_LTLIBRARIES = libpixman.la libcairo.la	# helper libs to compile .lo files

CAIRO_LOCATION = ./cairo-1.0.2/

%.c: ${CAIRO_LOCATION}/pixman/src/%.c
	cp $< $@
%.h: ${CAIRO_LOCATION}/pixman/src/%.h
	cp $< $@
%.c: ${CAIRO_LOCATION}/src/%.c
	cp $< $@
%.h: ${CAIRO_LOCATION}/src/%.h
	cp $< $@
%.h.in: ${CAIRO_LOCATION}/src/%.h.in
	cp $< $@
copy-files-on-demand: ${libpixman_la_SOURCES} ${libcairo_la_SOURCES} ${extra_files}
copy-files:
	$(MAKE) copy-files-on-demand
clean-files:
	rm -f $(strip ${libpixman_la_SOURCES} ${libcairo_la_SOURCES} ${extra_files})

${libpixman_la_OBJECTS}: | cairo-features.h copy-files
${libcairo_la_OBJECTS}:  | cairo-features.h copy-files
CLEANFILES += cairo-features.h

.PHONY: copy-files clean-files
clean: clean-files

EXTRA_DIST = config.h ${extra_files}

extra_files = cairo-features.h.in

libpixman_la_SOURCES = \
	pixman.h \
	pixman-remap.h \
	pixman-xserver-compat.h \
	pixregion.c \
	pixregionint.h \
	fbpict.h \
	fbpict.c \
	icblt.c \
	icbltone.c \
	iccolor.c \
	icformat.c \
	icimage.c \
	icimage.h \
	icint.h \
	icpixels.c \
	icrect.c \
	icrop.h \
	icstipple.c \
	ictrap.c \
	ictransform.c \
	ictri.c \
	icutil.c \
	fbedge.c \
	fbedgeimp.h \
	fbtrap.c \
	fbcompose.c \
	renderedge.c \
	renderedge.h \
	slim_internal.h fbmmx.h

libcairo_la_SOURCES = 				\
	cairo.c					\
	cairo.h					\
	cairo-private.h				\
	cairo-arc.c				\
	cairo-arc-private.h			\
	cairo-array.c				\
	cairo-cache.c				\
	cairo-clip.c				\
	cairo-clip-private.h			\
	cairo-color.c				\
	cairo-debug.c				\
	cairo-debug.h				\
	cairo-fixed.c				\
	cairo-font.c				\
	cairo-font-options.c			\
	cairo-gstate.c				\
	cairo-gstate-private.h			\
	cairo-hash.c				\
	cairo-hash-private.h			\
	cairo-hull.c				\
	cairo-image-surface.c			\
	cairo-matrix.c				\
	cairo-path.c				\
	cairo-path-bounds.c			\
	cairo-path-data.c			\
	cairo-path-data-private.h		\
	cairo-path-fill.c			\
	cairo-path-fixed-private.h		\
	cairo-path-stroke.c			\
	cairo-pen.c				\
	cairo-polygon.c				\
	cairo-region.c				\
	cairo-slope.c				\
	cairo-spline.c				\
	cairo-surface.c				\
	cairo-traps.c				\
	cairo-pattern.c				\
	cairo-unicode.c				\
	cairo-output-stream.c			\
	cairo-wideint.c				\
	cairo-wideint.h				\
	cairo-meta-surface.c			\
	cairo-meta-surface-private.h		\
	cairo-ft.h cairo-ft-font.c cairo-ft-private.h \
	cairo-xlib-surface.c cairo-xlib.h cairo-xlib-xrender.h cairo-xlib-test.h cairo-xlib-private.h \
	cairo-xlib-screen.c \
	cairo-png.c	\
	cairoint.h
	
