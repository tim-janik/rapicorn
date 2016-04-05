# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS    = . tests

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE -D__RAPICORN_BUILD__
AM_CXXFLAGS += $(RAPICORN_GUI_CFLAGS)

# === Source Files ===
rapicorn_public_headers = $(strip	\
	adjustment.hh	application.hh	arrangement.hh	\
	binding.hh	buttons.hh	\
	cmdlib.hh			commands.hh			container.hh	\
	evaluator.hh	events.hh	factory.hh	heritage.hh	\
	image.hh	widget.hh			keysymbols.hh	\
	layoutcontainers.hh	listarea.hh	models.hh	object.hh	\
	paintcontainers.hh	painter.hh	paintwidgets.hh	primitives.hh	\
	pixmap.hh	\
	region.hh	\
	scrollwidgets.hh	selector.hh	selob.hh	\
	sinfex.hh	sizegroup.hh	slider.hh	\
	stock.hh	style.hh	\
	table.hh	testwidgets.hh	text-editor.hh	text-pango.hh	\
	uithread.hh	utilities.hh	viewport.hh	window.hh	\
	displaywindow.hh	\
	$(rapicorn_impl_headers)	internal.hh	\
)
rapicorn_cc_sources = $(strip	serverglue.cc   clientglue.cc \
	adjustment.cc	application.cc	arrangement.cc	\
	blit-mmx.cc	blitfuncs.cc	\
	binding.cc	buttons.cc	\
	cmdlib.cc			commands.cc			container.cc	\
	evaluator.cc	events.cc	factory.cc	heritage.cc	\
	image.cc	widget.cc	\
	layoutcontainers.cc	listarea.cc	models.cc	object.cc	\
	paintcontainers.cc	painter.cc	paintwidgets.cc	primitives.cc	\
	pixmap.cc			\
	region.cc	regionimpl.c	\
	scrollwidgets.cc	selector.cc	selob.cc	\
	sinfex.cc	sizegroup.cc	slider.cc	\
	stock.cc	style.cc	\
	table.cc	testwidgets.cc	text-editor.cc	text-pango.cc	\
	uithread.cc	utilities.cc	viewport.cc	window.cc	\
	displaywindow-x11.cc	displaywindow.cc	\
)
rapicorn_impl_headers = $(strip 	\
	sinfeximpl.hh			\
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
	displaywindow-xaux.cc		\
	serverapi.cc			\
)
EXTRA_DIST += widget.dox

# == Special Optimization Targets ==
OPTIMIZE_SOURCE_FILES = serverglue.cc clientglue.cc
AM_CXXFLAGS += $(patsubst %, @OPTIMIZE_FAST@, $(findstring $(<F), $(OPTIMIZE_SOURCE_FILES)))

# === CPU SIMD Flags ===
AM_CXXFLAGS += $(patsubst %, @OPTIMIZE_FAST@ @SSEAVX_FLAGS@, $(findstring -mmx.cc, $(<F)))

# === Libraries ===
# librapicorn
lib_LTLIBRARIES                     = librapicorn-@MAJOR@.la
LDSCRIPT_MAP                        = $(top_srcdir)/rcore/ldscript.map
librapicorn_@MAJOR@includedir       = $(includedir)/rapicorn-@MAJOR@/ui
librapicorn_@MAJOR@include_HEADERS  = $(rapicorn_public_headers) $(rapicorn_generated_headers)
librapicorn_@MAJOR@_la_SOURCES      = $(rapicorn_cc_sources)
librapicorn_@MAJOR@_la_DEPENDENCIES = $(top_builddir)/rcore/librapicorncore-@MAJOR@.la ${LDSCRIPT_MAP}
librapicorn_@MAJOR@_la_LDFLAGS      = $(strip		\
	$(AM_LDFLAGS) -no-undefined			\
	-Wl,--version-script=${LDSCRIPT_MAP}		\
	-version-info $(LT_VERSION_INFO) 		\
) # set libtool version and export symbols for resolving
librapicorn_@MAJOR@_la_LIBADD       = $(strip			\
	$(top_builddir)/rcore/librapicorncore-@MAJOR@.la	\
	$(RAPICORN_GUI_LIBS) $(SVG_DEPENDENCIES_LIBS)		\
	$(LIBXML_LIBS) -lm					\
) # -lfreetype -lfontconfig
EXTRA_DIST += $(rapicorn_private_files)
# -- Build OBJECTS alphabetically --
librapicorn1008_la_OBJECTS := $(sort $(librapicorn1008_la_OBJECTS))

install-exec-ldconfig: # libtool >= 1.5.2 doesn't always set rpath, requiring ldconfig
	test ! -x /sbin/ldconfig -o ! -w /etc/ld.so.cache || /sbin/ldconfig $(DESTDIR)$(libdir)
install-exec-hook: install-exec-ldconfig

# === Rapicorn IDL ===
AIDACC         = $(top_builddir)/aidacc/aidacc-intern
CXXSTUB_PY     = $(top_srcdir)/aidacc/CxxStub.py
EXTRACTDOCS_PY = $(top_srcdir)/aidacc/ExtractDocs.py
EXTRA_DIST    += interfaces.idl interfaces.dox serverinsert.hh
# generate interfaces.dox to complete docs
$(rapicorn_cc_sources): interfaces.dox
interfaces.dox: $(EXTRACTDOCS_PY) $(srcdir)/interfaces.idl
	${AIDACC} -x ExtractDocs $(srcdir)/interfaces.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
CLEANFILES += interfaces.dox
# build serverapi.{hh|cc} and clientapi.{hh|cc} from interfaces.idl
$(rapicorn_cc_sources): serverapi.hh clientapi.hh
serverapi.hh: $(CXXSTUB_PY) $(srcdir)/interfaces.idl $(srcdir)/serverinsert.hh
	${AIDACC} -x CxxStub -G macro=RAPICORN_IDL -G serverhh \
	  --insertions $(srcdir)/serverinsert.hh $(srcdir)/interfaces.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
serverglue.cc: serverapi.cc
serverapi.cc: $(CXXSTUB_PY) $(srcdir)/interfaces.idl serverapi.hh
	${AIDACC} -x CxxStub -G macro=RAPICORN_IDL -G servercc -G strip-path=$(abs_top_srcdir)/ \
	  --inclusions '"widget.hh"' $(srcdir)/interfaces.idl -o - >> xgen-$(@F)
	mv xgen-$(@F) $@
CLEANFILES += serverapi.cc serverapi.hh
EXTRA_DIST += clientinsert.hh
clientapi.hh: $(CXXSTUB_PY) $(srcdir)/interfaces.idl $(srcdir)/clientinsert.hh
	${AIDACC} -x CxxStub -G macro=RAPICORN_IDL -G clienthh \
	  --insertions $(srcdir)/clientinsert.hh $(srcdir)/interfaces.idl -o xgen-$(@F)
	mv xgen-$(@F) $@
clientglue.cc: clientapi.cc
clientapi.cc: $(CXXSTUB_PY) $(srcdir)/interfaces.idl clientapi.hh
	${AIDACC} -x CxxStub -G macro=RAPICORN_IDL -G clientcc -G strip-path=$(abs_top_srcdir)/ \
	  --inclusions '"clientapi.hh"' $(srcdir)/interfaces.idl -o - >> xgen-$(@F)
	mv xgen-$(@F) $@
CLEANFILES += clientapi.cc clientapi.hh
# Choose either ClnT or SrvT implementation for enums to avoid duplicate symbols
AM_CXXFLAGS += $(patsubst %, -D__ENUMCC__RAPICORN_IDL__=1, $(findstring $(<F), clientglue.cc serverglue.cc))

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
progs_ldadd            = librapicorn-@MAJOR@.la $(RAPICORN_GUI_LIBS) -lm

# === XML Files ===
XML_FILES = $(top_srcdir)/res/Rapicorn/foundation.xml $(top_srcdir)/res/Rapicorn/standard.xml
xmllint-check:
	! $(XMLLINT) --noout $(XML_FILES) 2>&1 | grep '.'
.PHONY: xmllint-check
check-local: xmllint-check
