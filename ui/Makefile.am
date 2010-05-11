# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS		= . tests

INCLUDES	+= -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS		 = -DRAPICORN_INTERNALS $(RAPICORN_GUI_CFLAGS) -DRAPICORN_LOG_DOMAIN=\"RAPICORN\" # "

# === Source Files ===
rapicorn_public_headers = $(strip 	\
	keycodes.hh	$(rapicorn_impl_headers)	\
	utilities.hh	events.hh	application.hh	cmdlib.hh	\
	primitives.hh	region.hh	loop.hh		viewport.hh	heritage.hh		\
	properties.hh	commands.hh			painter.hh	window.hh		\
	item.hh		container.hh	root.hh		table.hh	arrangement.hh		\
	adjustment.hh	image.hh	paintitems.hh	slider.hh	paintcontainers.hh	\
	buttons.hh	factory.hh	evaluator.hh	scrollitems.hh	layoutcontainers.hh	\
	text-editor.hh	text-pango.hh	sinfex.hh	testitems.hh	\
	sizegroup.hh	listarea.hh	ui.hh \
)
rapicorn_impl_headers = $(strip 	\
	arrangementimpl.hh		\
	containerimpl.hh		\
	itemimpl.hh			\
	listareaimpl.hh			\
	rootimpl.hh			\
	scrollitemsimpl.hh		\
	sinfeximpl.hh			\
	tableimpl.hh			\
)
rapicorn_generated_headers = $(strip	\
	interface.hh			\
)
rapicorn_cc_sources = $(strip 		\
	\
	viewport-gtk.cc	regionimpl.c	blitfuncs.cc	blit-mmx.cc	\
	utilities.cc	events.cc	application.cc	cmdlib.cc	\
	primitives.cc	region.cc	loop.cc		viewport.cc	heritage.cc		\
	properties.cc	commands.cc			painter.cc	window.cc		\
	item.cc		container.cc	root.cc		table.cc	arrangement.cc		\
	adjustment.cc	image.cc	paintitems.cc	slider.cc	paintcontainers.cc	\
	buttons.cc	factory.cc	evaluator.cc	scrollitems.cc	layoutcontainers.cc	\
	text-editor.cc	text-pango.cc	sinfex.cc	testitems.cc	\
	sizegroup.cc	listarea.cc	\
	interface.cc	\
)
rapicorn_private_files = $(strip 	\
	blitfuncs.hh			\
	regionimpl.h			\
	key2utf8.cc			\
)

# === CPU flags ===
CPU_FLAGS = $(or	\
	$(patsubst %, @MMX_FLAGS@, $(findstring -mmx.cc, $(<F))), \
	@FPU_FLAGS@	\
)
AM_CXXFLAGS = $(CPU_FLAGS)

# === Libraries ===
# librapicorn
lib_LTLIBRARIES = librapicorn@RAPICORN_RELEASE@.la	# hardcoded -release $(RAPICORN_RELEASE)
librapicorn@RAPICORN_RELEASE@includedir       = $(includedir)/rapicorn@RAPICORN_RELEASE@/ui
librapicorn@RAPICORN_RELEASE@include_HEADERS  = $(rapicorn_public_headers) $(rapicorn_generated_headers)
librapicorn@RAPICORN_RELEASE@_la_SOURCES      = $(rapicorn_cc_sources)
librapicorn@RAPICORN_RELEASE@_la_DEPENDENCIES = $(top_builddir)/rcore/librapicorncore@RAPICORN_RELEASE@.o ldscript.map
librapicorn@RAPICORN_RELEASE@_la_LDFLAGS      = $(strip		\
	-Wl,--version-script=$(srcdir)/ldscript.map		\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) 	\
	-no-undefined $(SYMBOLIC_LDFLAGS)			\
) # set libtool version and export symbols for resolving
librapicorn@RAPICORN_RELEASE@_la_LIBADD       = $(top_builddir)/rcore/librapicorncore@RAPICORN_RELEASE@.o $(RAPICORN_GUI_LIBS) -lm # -lfreetype -lfontconfig
EXTRA_DIST += $(rapicorn_private_files) $(librapicorn@RAPICORN_RELEASE@_la_DEPENDENCIES)

# === Rapicorn IDL ===
ROPE        = $(srcdir)/../rope
PLICINT     = $(top_builddir)/plic/plic-intern
EXTRA_DIST += rapicorn.idl insertions.cc hardcoded.idl
interface.hh: $(ROPE)/CxxCaller.py $(srcdir)/rapicorn.idl $(srcdir)/insertions.cc
	: # build interface.hh from rapicorn.idl
	${PLICINT} -G $(ROPE)/CxxCaller -g interface \
	  --insertions $(srcdir)/insertions.cc $(srcdir)/rapicorn.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
$(rapicorn_cc_sources): interface.hh
CLEANFILES += xgen-interface.hh interface.hh
interface.cc: $(ROPE)/CxxCaller.py $(srcdir)/rapicorn.idl
	: # build interface.cc from rapicorn.idl
	${PLICINT} -G $(ROPE)/CxxCaller -g interface-impl \
	  --inclusions '"item.hh"' $(srcdir)/rapicorn.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
CLEANFILES += interface.cc
# generate skeletons for non-virtual methods, skipping existing symbols
skeletons.cc: $(librapicorn@RAPICORN_RELEASE@_la_OBJECTS)
	for obj in $(librapicorn@RAPICORN_RELEASE@_la_OBJECTS) ; do case "$$obj" in \
	  *.lo) . "./$$obj" ; o="$$pic_object" ;; *.o|*) o="$$obj" ;; esac ; \
	  nm -C "$$o" ; done | grep -v '^[[:space:]]*U[[:space:]]' | \
	  sed 's/\b@RAPICORN_NAMESPACE_NAME@::/Rapicorn::/g' > xgen-syms-$(@F)
	${PLICINT} -G $(ROPE)/CxxCaller -g skeletons.cc --skip-skels xgen-syms-$(@F) \
	  -g interface-skel $(srcdir)/rapicorn.idl -o xgen-$(@F)
	rm -f xgen-syms-$(@F)
	mv xgen-$(@F) $@
$(LTLIBRARIES): skeletons.cc

# === API Headers ===
api-content:
	cat $(sort $(rapicorn_public_headers) $(rapicorn_generated_headers))
.PHONY: api-content

# === Flex & Bison rules ===
EXTRA_DIST += sinfex.l
GENERATED_EXTRA_DIST += sinfex.lgen
sinfex.lgen: sinfex.l
	flex -CFe -o$@ $<
EXTRA_DIST += sinfex.y
GENERATED_EXTRA_DIST += sinfex.ygen
sinfex.ygen: sinfex.y
	bison -o$@ $<
sinfex.cc: sinfex.lgen sinfex.ygen

# === Programs (not installed) ===
noinst_PROGRAMS        = $(ALLTESTS)
progs_ldadd            = librapicorn@RAPICORN_RELEASE@.la $(RAPICORN_GUI_LIBS) -lm

# === XML Files ===
XML_FILES   = foundation.xml standard.xml
EXTRA_DIST += $(XML_FILES)
xmllint-check:
	cd $(srcdir) && $(XMLLINT) --noout $(XML_FILES)
.PHONY: xmllint-check
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xmllint-check
	@touch $@
noinst_DATA = stamp-autochecks
CLEANFILES += stamp-autochecks

# === ZIntern generation ===
gen-zintern.c:	../rcore/rapicorn-zintern $(srcdir)/foundation.xml $(srcdir)/standard.xml stamp-autochecks
	../rcore/rapicorn-zintern -b -z FOUNDATION_XML \
	  $(srcdir)/foundation.xml STANDARD_XML $(srcdir)/standard.xml > xgen-$(@F)
	mv xgen-$(@F) $@
$(srcdir)/factory.cc: gen-zintern.c
CLEANFILES += xgen-gen-zintern.c gen-zintern.c
