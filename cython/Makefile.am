# Rapicorn Cython
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_GUI_CFLAGS) $(PYTHON_CPPFLAGS)

AIDACC       = $(top_builddir)/aidacc/aidacc-intern
RAPICORNIDL  = $(srcdir)/../ui/interfaces.idl
HARDCODEDIDL = $(srcdir)/../ui/hardcoded.idl

# Py Rapicorn - installation
Rapicorndir         	= ${pythondir}/@RAPICORN_NAMESPACE_NAME@
Rapicorn_PYTHON 	= # __init__.py
Rapicorn_LTLIBRARIES	= Rapicorn.la
# Pyx Rapicorn module
Rapicorn_la_SOURCES	= Rapicorn.cc
Rapicorn_la_CXXFLAGS	= $(AM_CXXFLAGS) -Wno-redundant-decls
Rapicorn_la_LDFLAGS	= $(AM_LDFLAGS) $(SYMBOLIC_LDFLAGS) -module -avoid-version -no-undefined
Rapicorn_la_LIBADD	= $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la
EXTRA_DIST	       += Rapicorn.pyx
CLEANFILES		= Rapicorn.cc

%.cc: %.pyx
	$(CYTHON) --cplus -Wextra -Werror $< -o $@

