# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS    = . tests

INCLUDES   += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS       += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE -D__RAPICORN_BUILD__
AM_CXXFLAGS = $(RAPICORN_GUI_CFLAGS)

# === Source Files ===
rapicorn_public_headers = $(strip	\
	adjustment.hh	application.hh	arrangement.hh	buttons.hh	\
	cmdlib.hh	coffer.hh	commands.hh			container.hh	\
	evaluator.hh	events.hh	factory.hh	heritage.hh	\
	image.hh	widget.hh		keysymbols.hh	\
	layoutcontainers.hh	listarea.hh	models.hh	\
	paintcontainers.hh	painter.hh	paintwidgets.hh	primitives.hh	\
	region.hh	\
	scrollwidgets.hh	selector.hh	selob.hh	\
	sinfex.hh	sizegroup.hh	slider.hh	\
	stock.hh	\
	table.hh	testwidgets.hh	text-editor.hh	text-pango.hh	\
	uithread.hh	utilities.hh	viewport.hh	window.hh	\
	screenwindow.hh	\
	$(rapicorn_impl_headers)	internal.hh	pixmap.hh	\
)
rapicorn_cc_sources = $(strip	serverglue.cc   clientglue.cc \
	adjustment.cc	application.cc	arrangement.cc	\
	blit-mmx.cc	blitfuncs.cc	buttons.cc	\
	cmdlib.cc	coffer.cc	commands.cc			container.cc	\
	evaluator.cc	events.cc	factory.cc	heritage.cc	\
	image.cc	widget.cc		\
	layoutcontainers.cc	listarea.cc	models.cc	\
	paintcontainers.cc	painter.cc	paintwidgets.cc	primitives.cc	\
	region.cc	regionimpl.c	\
	scrollwidgets.cc	selector.cc	selob.cc	\
	sinfex.cc	sizegroup.cc	slider.cc	\
	stock.cc	\
	table.cc	testwidgets.cc	text-editor.cc	text-pango.cc	\
	uithread.cc	utilities.cc	viewport.cc	window.cc	\
	screenwindow-x11.cc	screenwindow.cc	\
)
rapicorn_impl_headers = $(strip 	\
	arrangementimpl.hh		\
	listareaimpl.hh			\
	scrollwidgetsimpl.hh		\
	sinfeximpl.hh			\
	tableimpl.hh			\
)
rapicorn_generated_headers = $(strip	\
	clientapi.hh			\
	serverapi.hh			\
)
rapicorn_private_files = $(strip 	\
	blitfuncs.hh			\
	regionimpl.h			\
	key2ucs4.cc			\
	clientapi.cc			\
	screenwindow-xaux.cc		\
	serverapi.cc			\
	pixmap.cc			\
)

# === CPU flags ===
CPU_FLAGS = $(or	\
	$(patsubst %, @MMX_FLAGS@, $(findstring -mmx.cc, $(<F))), \
	@FPU_FLAGS@	\
)
AM_CXXFLAGS += $(CPU_FLAGS)

# === Libraries ===
# librapicorn
lib_LTLIBRARIES = librapicorn@RAPICORN_RELEASE@.la	# hardcoded -release $(RAPICORN_RELEASE)
librapicorn@RAPICORN_RELEASE@includedir       = $(includedir)/rapicorn@RAPICORN_RELEASE@/ui
librapicorn@RAPICORN_RELEASE@include_HEADERS  = $(rapicorn_public_headers) $(rapicorn_generated_headers)
librapicorn@RAPICORN_RELEASE@_la_SOURCES      = $(rapicorn_cc_sources)
librapicorn@RAPICORN_RELEASE@_la_DEPENDENCIES = $(top_builddir)/rcore/librapicorncore@RAPICORN_RELEASE@.o ldscript.map \
						$(top_builddir)/rcore/rsvg/librcore_rsvg@RAPICORN_RELEASE@.o
librapicorn@RAPICORN_RELEASE@_la_LDFLAGS      = $(strip		\
	-Wl,--version-script=$(srcdir)/ldscript.map		\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) 	\
	-no-undefined $(SYMBOLIC_LDFLAGS)			\
) # set libtool version and export symbols for resolving
librapicorn@RAPICORN_RELEASE@_la_LIBADD       = $(strip		\
	$(top_builddir)/rcore/librapicorncore@RAPICORN_RELEASE@.o \
	$(top_builddir)/rcore/rsvg/librcore_rsvg@RAPICORN_RELEASE@.o \
	$(LIBXML_LIBS) \
	$(RAPICORN_GUI_LIBS) -lm \
) # -lfreetype -lfontconfig
EXTRA_DIST += $(rapicorn_private_files) $(librapicorn@RAPICORN_RELEASE@_la_DEPENDENCIES)
# -- Build OBJECTS alphabetically --
librapicorn1008_la_OBJECTS := $(sort $(librapicorn1008_la_OBJECTS))

# === Rapicorn IDL ===
ROPE        = $(srcdir)/../rope
AIDACC      = $(top_builddir)/aidacc/aidacc-intern
CXXSTUB_PY  = $(top_srcdir)/aidacc/CxxStub.py
EXTRA_DIST += interfaces.idl serverinsert.hh hardcoded.idl
# build serverapi.{hh|cc} and clientapi.{hh|cc} from interfaces.idl
$(rapicorn_cc_sources): serverapi.hh clientapi.hh
serverapi.hh: $(CXXSTUB_PY) $(srcdir)/interfaces.idl $(srcdir)/serverinsert.hh
	${AIDACC} -x CxxStub -G serverhh -G iface-base=Rapicorn::ReferenceCountable -G iface-postfix=Iface \
	  -G cppguard=__RAPICORN_SERVERAPI_HH_ \
	  --insertions $(srcdir)/serverinsert.hh $(srcdir)/interfaces.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
serverglue.cc: serverapi.cc
serverapi.cc: $(CXXSTUB_PY) $(srcdir)/interfaces.idl serverapi.hh
	${AIDACC} -x CxxStub -G servercc -G iface-base=Rapicorn::ReferenceCountable -G iface-postfix=Iface \
	  --inclusions '"widget.hh"' $(srcdir)/interfaces.idl -o - >> xgen-$(@F)
	mv xgen-$(@F) $@
CLEANFILES += serverapi.cc serverapi.hh
EXTRA_DIST += clientinsert.hh
clientapi.hh: $(CXXSTUB_PY) $(srcdir)/interfaces.idl $(srcdir)/clientinsert.hh
	${AIDACC} -x CxxStub -G clienthh -G iface-base=Rapicorn::ReferenceCountable -G iface-postfix=Iface \
	  -G cppguard=__RAPICORN_CLIENTAPI_HH_ \
	  --insertions $(srcdir)/clientinsert.hh $(srcdir)/interfaces.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
clientglue.cc: clientapi.cc
clientapi.cc: $(CXXSTUB_PY) $(srcdir)/interfaces.idl clientapi.hh
	${AIDACC} -x CxxStub -G clientcc -G iface-base=Rapicorn::ReferenceCountable -G iface-postfix=Iface \
	  --inclusions '"clientapi.hh"' $(srcdir)/interfaces.idl -o - >> xgen-$(@F)
	mv xgen-$(@F) $@
CLEANFILES += clientapi.cc clientapi.hh
AM_CXXFLAGS += $(patsubst %, -O3 -minline-all-stringops -finline-functions, $(findstring $(<F), serverglue.cc clientglue.cc))
# generate skeletons for non-virtual methods, skipping existing symbols
skeletons.cc: $(CXXSTUB_PY) $(librapicorn@RAPICORN_RELEASE@_la_OBJECTS)
	for obj in $(librapicorn@RAPICORN_RELEASE@_la_OBJECTS) ; do case "$$obj" in \
	  *.lo) . "./$$obj" ; o="$$pic_object" ;; *.o|*) o="$$obj" ;; esac ; \
	  nm -C "$$o" ; done | grep -v '^[[:space:]]*U[[:space:]]' | \
	  sed 's/\b@RAPICORN_NAMESPACE_NAME@::/Rapicorn::/g' > xgen-syms-$(@F)
	${AIDACC} -x CxxStub -G server-skel -G iface-postfix=Iface --skip-skels xgen-syms-$(@F) \
	  $(srcdir)/interfaces.idl -o xgen-$(@F)
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
	$(FLEX) -CFe -o$@ $<
EXTRA_DIST += sinfex.y
GENERATED_EXTRA_DIST += sinfex.ygen
sinfex.ygen: sinfex.y
	$(BISON) -o$@ $<
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
