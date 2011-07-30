# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS    = . tests

INCLUDES += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS      = -DRAPICORN_CONVENIENCE -D__SOURCE_FILE__='"$(patsubst $(realpath $(abs_top_srcdir))/%,%, $(realpath $<))"' \
	    $(RAPICORN_GUI_CFLAGS) -D__RAPICORN_BUILD__

# === Source Files ===
rapicorn_public_headers = $(strip	\
	adjustment.hh	application.hh	arrangement.hh	buttons.hh	\
	cmdlib.hh	coffer.hh	commands.hh	compath.hh	container.hh	\
	evaluator.hh	events.hh	factory.hh	heritage.hh	\
	image.hh	item.hh	keycodes.hh	layoutcontainers.hh	listarea.hh	\
	paintcontainers.hh	painter.hh	paintitems.hh	primitives.hh	properties.hh	\
	region.hh	rope.hh	scrollitems.hh	sinfex.hh	sizegroup.hh	slider.hh	\
	table.hh	testitems.hh	text-editor.hh	text-pango.hh	\
	uithread.hh	utilities.hh	viewp0rt.hh	window.hh	\
	$(rapicorn_impl_headers)	\
)
rapicorn_cc_sources = $(strip	serverglue.cc   clientglue.cc \
	adjustment.cc	application.cc	arrangement.cc	\
	blit-mmx.cc	blitfuncs.cc	buttons.cc	\
	cmdlib.cc	coffer.cc	commands.cc	compath.cc	container.cc	\
	evaluator.cc	events.cc	factory.cc	heritage.cc	\
	image.cc	item.cc		\
	layoutcontainers.cc	listarea.cc	\
	paintcontainers.cc	painter.cc	paintitems.cc	primitives.cc	properties.cc	\
	region.cc	regionimpl.c	rope.cc	\
	scrollitems.cc	sinfex.cc	sizegroup.cc	slider.cc	\
	table.cc	testitems.cc	text-editor.cc	text-pango.cc	\
	uithread.cc	utilities.cc	viewp0rt-gtk.cc	viewp0rt.cc	window.cc	\
)
rapicorn_impl_headers = $(strip 	\
	arrangementimpl.hh		\
	listareaimpl.hh			\
	scrollitemsimpl.hh		\
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
	key2utf8.cc			\
	clientapi.cc			\
	serverapi.cc			\
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

# === Rapicorn IDL ===
ROPE        = $(srcdir)/../rope
PLICINT     = $(top_builddir)/plic/plic-intern
EXTRA_DIST += interfaces.idl serverinsert.cc hardcoded.idl
# build interfaces.{hh|cc} and clientapi.{hh|cc} from interfaces.idl
$(rapicorn_cc_sources): serverapi.hh clientapi.hh
serverapi.hh: $(ROPE)/CxxStub.py $(srcdir)/interfaces.idl $(srcdir)/serverinsert.cc
	${PLICINT} -G $(ROPE)/CxxStub -g serverhh -g iface-base=BaseObject -g iface-postfix=Iface \
	  -g shortalias -g cppguard=__RAPICORN_INTERFACE_HH_ \
	  --insertions $(srcdir)/serverinsert.cc $(srcdir)/interfaces.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
serverglue.cc: serverapi.cc
serverapi.cc: $(ROPE)/CxxStub.py $(srcdir)/interfaces.idl serverapi.hh
	echo "#define PLIC_CALL_REMOTE // dummy" > xgen-$(@F)
	${PLICINT} -G $(ROPE)/CxxStub -g servercc -g iface-base=BaseObject -g iface-postfix=Iface \
	  --inclusions '"item.hh"' $(srcdir)/interfaces.idl -o - >> xgen-$(@F)
	mv xgen-$(@F) $@
CLEANFILES += serverapi.cc serverapi.hh
EXTRA_DIST += clientinsert.cc
clientapi.hh: $(ROPE)/CxxStub.py $(srcdir)/interfaces.idl $(srcdir)/clientinsert.cc
	${PLICINT} -G $(ROPE)/CxxStub -g clienthh -g iface-base=BaseObject -g iface-postfix=Iface \
	  -g shortalias -g cppguard=__RAPICORN_CLIENTAPI_HH_ \
	  --insertions $(srcdir)/clientinsert.cc $(srcdir)/interfaces.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
clientglue.cc: clientapi.cc
clientapi.cc: $(ROPE)/CxxStub.py $(srcdir)/interfaces.idl clientapi.hh
	echo "#define PLIC_CALL_REMOTE // dummy" > xgen-$(@F)
	${PLICINT} -G $(ROPE)/CxxStub -g clientcc -g iface-base=BaseObject -g iface-postfix=Iface \
	  --inclusions '"clientapi.hh"' $(srcdir)/interfaces.idl -o - >> xgen-$(@F)
	mv xgen-$(@F) $@
CLEANFILES += clientapi.cc clientapi.hh
# generate skeletons for non-virtual methods, skipping existing symbols
skeletons.cc: $(librapicorn@RAPICORN_RELEASE@_la_OBJECTS)
	for obj in $(librapicorn@RAPICORN_RELEASE@_la_OBJECTS) ; do case "$$obj" in \
	  *.lo) . "./$$obj" ; o="$$pic_object" ;; *.o|*) o="$$obj" ;; esac ; \
	  nm -C "$$o" ; done | grep -v '^[[:space:]]*U[[:space:]]' | \
	  sed 's/\b@RAPICORN_NAMESPACE_NAME@::/Rapicorn::/g' > xgen-syms-$(@F)
	${PLICINT} -G $(ROPE)/CxxStub -g server-skel -g iface-postfix=Iface \
	  --skip-skels xgen-syms-$(@F) \
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
