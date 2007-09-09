# Rapicorn
# Copyright (C) 2005 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

SUBDIRS = . tests

INCLUDES       += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I. $(PANGO_CFLAGS) $(GTK_CFLAGS)
GLIB_MKENUMS    = glib-mkenums
LDLIBS	        = $(LIBS) $(PANGO_LIBS) $(GTK_LIBS)
DEFS            = -DRAPICORN_PRIVATE -DPARANOID -D_RAPICORN_SOURCE_EXTENSIONS -DRAPICORN_LOG_DOMAIN=\"RAPICORN\" # "


rapicorn_top_headers = rapicorn.hh
rapicorn_public_headers = $(strip 	\
	keycodes.hh	$(rapicorn_impl_headers)	\
	utilities.hh	events.hh	application.hh	\
	primitives.hh	region.hh	loop.hh		viewport.hh		\
	properties.hh	commands.hh	appearance.hh	painter.hh	window.hh		\
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
	scrollitemsimpl.hh		\
	tableimpl.hh			\
)
rapicorn_generated_headers = $(strip	\
)
rapicorn_cc_sources = $(strip 		\
	\
	viewport-gtk.cc	regionimpl.c	\
	utilities.cc	events.cc	application.cc	\
	primitives.cc	region.cc	loop.cc		viewport.cc		\
	properties.cc	commands.cc	appearance.cc	painter.cc	window.cc		\
	item.cc		container.cc	root.cc		table.cc	arrangement.cc		\
	adjustment.cc	image.cc	paintgadgets.cc	slider.cc	paintcontainers.cc	\
	buttons.cc	factory.cc	evaluator.cc	scrollitems.cc	layoutcontainers.cc	\
	text-editor.cc	text-pango.cc	testitem.cc	\
)
rapicorn_private_files = $(strip 	\
                  \
	regionimpl.h			\
	private.hh			\
	key2utf8.cc			\
)

# libraries
topincludedir   = $(includedir)/rapicorn@LT_RELEASE@
topinclude_DATA = $(rapicorn_top_headers)
EXTRA_DIST     += $(rapicorn_top_headers)
# librapicorn
lib_LTLIBRARIES                         = librapicorn@LT_RELEASE@.la	# hardcoded -release $(LT_RELEASE)
librapicorn@LT_RELEASE@includedir       = $(includedir)/rapicorn@LT_RELEASE@/rapicorn
librapicorn@LT_RELEASE@include_HEADERS  = $(rapicorn_public_headers) $(rapicorn_generated_headers)
librapicorn@LT_RELEASE@_la_SOURCES      = $(rapicorn_cc_sources)
librapicorn@LT_RELEASE@_la_DEPENDENCIES = ldscript.map
librapicorn@LT_RELEASE@_la_LDFLAGS      = $(strip		\
	-Wl,--version-script=$(srcdir)/ldscript.map		\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) 	\
	-no-undefined 						\
) # set libtool version and export symbols for resolving
librapicorn@LT_RELEASE@_la_LIBADD       = $(top_builddir)/rapicorn-core/librapicorn@LT_RELEASE@core.o $(RAPICORN_LIBS) -lm # -lfreetype -lfontconfig
EXTRA_DIST += $(rapicorn_private_files) $(librapicorn@LT_RELEASE@_la_DEPENDENCIES)

# programs (not installed)
noinst_PROGRAMS        = $(ALLTESTS)
progs_ldadd            = librapicorn@LT_RELEASE@.la $(RAPICORN_LIBS) -lm


# === header-checksum ===
RAPICORN_MAKE_CHECKSUM=$(strip			\
	cat $(filter-out rapicornconfig.h,	\
		../rapicorn-core/*.hh		\
		../rapicorn-core/*.h		\
		$(rapicorn_public_headers)	\
		$(rapicorn_top_headers)		\
		$(rapicorn_generated_headers)	\
	) | md5sum | sed 's/ .*//'		\
)
api-check:	# ensure headers don't change if RELEASE=outdated && REVISION=0
	cd $(srcdir) \
	&& RAPICORN_CURRENT_CHECKSUM=$$($(RAPICORN_MAKE_CHECKSUM))		\
	&& RAPICORN_WEEK_VERSION="$$(expr $$(date +%G) - 2000)$$(date +%V)"	\
	&& test "$$RAPICORN_CURRENT_CHECKSUM"  = "`cat api-check.md5`"		\
	|| if [ "@RAPICORN_RELEASE_VERSION@"   = "$$RAPICORN_WEEK_VERSION" -o	\
		"@RAPICORN_REVISION_VERSION@" -gt 0 ]; then			\
	     echo "$$RAPICORN_CURRENT_CHECKSUM" > api-check.md5 ;		\
	   else \
	     MSG1="The Rapicorn version (@RAPICORN_RELEASE_VERSION@.@RAPICORN_REVISION_VERSION@) needs updating " ;		\
	     MSG2="(expected $$RAPICORN_WEEK_VERSION.0) to match header changes (checksum: $$RAPICORN_CURRENT_CHECKSUM)" ;	\
	     MSG="$$MSG1$$MSG2" ; echo $$MSG | sed 's/./=/g'; echo $$MSG; echo $$MSG | sed 's/./=/g' ;				\
	     exit 1 ; \
	   fi
api-checksum:
	$(RAPICORN_MAKE_CHECKSUM)
#check-am: api-check # this is called by the toplevel Makefile.am
EXTRA_DIST += api-check.md5
.PHONY: api-check api-checksum

#XML_FILES = $(strip standard.xml )
## xml linting
#stamp-xmllint: $(XML_FILES)
#	cd $(srcdir) \
#	&& $(XMLLINT) --noout $(XML_FILES)
#	touch $@
#$(lib_LTLIBRARIES): stamp-xmllint

XML_FILES   = foundation.xml
XMLLINTCALL = $(if $(XMLLINT),$(XMLLINT),true) # handle undefined XMLLINT
xml-lint:
	$(XMLLINTCALL) --noout $(XML_FILES)
.PHONY: xml-lint
stamp-autochecks: $(XML_FILES)
	@$(MAKE) xml-lint
	@touch $@
noinst_DATA = stamp-autochecks
EXTRA_DIST += $(XML_FILES)

gen-zintern.c:	../rapicorn-core/rapicorn-zintern $(srcdir)/foundation.xml stamp-autochecks
	../rapicorn-core/rapicorn-zintern -b -z FOUNDATION_XML $(srcdir)/foundation.xml > xgen-$(@F)
	mv xgen-$(@F) $@
factory.cc: gen-zintern.c
CLEANFILES += xgen-gen-zintern.c gen-zintern.c
