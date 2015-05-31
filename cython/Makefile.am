# Rapicorn Cython
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_GUI_CFLAGS) $(PYTHON_CPPFLAGS)

AIDACC       = $(top_builddir)/aidacc/aidacc-intern
RAPICORNIDL  = $(srcdir)/../ui/interfaces.idl
# Loading Python modules requires accesss to libtool objdir
objdir = .libs
RPYTHON      = PYTHONPATH="$(abs_builddir)/$(objdir):$$PYTHONPATH" $(PYTHON)
XMANIFOLD    = $(top_srcdir)/aidacc/xmanifold.py

# Py Rapicorn - installation
Rapicorndir         	= ${pythondir}/@RAPICORN_NAMESPACE_NAME@
Rapicorn_PYTHON 	= # __init__.py
Rapicorn_LTLIBRARIES	= Rapicorn.la
# Pyx Rapicorn module
Rapicorn_la_SOURCES	= Rapicorn.cc
Rapicorn_la_CXXFLAGS	= $(AM_CXXFLAGS) -Wno-redundant-decls -Wno-unused-function
Rapicorn_la_LDFLAGS	= $(AM_LDFLAGS) -no-undefined -module -avoid-version
Rapicorn_la_LIBADD	= $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la
EXTRA_DIST	       += Rapicorn.pyx pyxxutils.hh pyxxutils.pyx rcore.pyx PyxxStub.py
CLEANFILES	       += Rapicorn.cc idlapi.pyx

Rapicorn.cc: Rapicorn.pyx pyxxutils.pyx rcore.pyx idlapi.pyx pyxxutils-xmany.hh
	@echo "  CYTHON  " $@
	$(Q) $(CYTHON) --cplus -Wextra -Werror --fast-fail Rapicorn.pyx -o $@
idlapi.pyx: $(RAPICORNIDL) $(srcdir)/PyxxStub.py
	@echo "  AIDACC  " $@
	$(Q) ${AIDACC} -x $(srcdir)/PyxxStub $(RAPICORNIDL) -o $@ -G strip-path=$(abs_top_srcdir)/
$(srcdir)/*.cc: pyxxutils-xmany.hh
pyxxutils-xmany.hh: $(srcdir)/pyxxutils.hh $(XMANIFOLD)
	$(AM_V_GEN)
	$(Q) $(XMANIFOLD) -S $(srcdir)/pyxxutils.hh 18 >xtmp-$(@F) \
	&& mv xtmp-$(@F) $@
CLEANFILES += pyxxutils-xmany.hh


# === Tests ===
TEST_PYPROGS = testing1.py

# === TEST_PYPROGS ===
.PHONY: check-TEST_PYPROGS
check-TEST_PYPROGS:
	$(Q) $(X11TEST_SERVER) && \
	  for tst in $(TEST_PYPROGS) ; do				\
	    [ -f ./"$$tst" ] && rt=./"$$tst" || rt="$(srcdir)/$$tst" ;	\
	    $(RPYTHON) $$rt && echo "PASS: $$tst" || exit $? ;		\
	  done
	@MESSAGETEXT="All $(words $(TEST_PYPROGS)) Python tests passed"	\
	&& [ 0 -lt $(words $(TEST_PYPROGS)) ]				\
	&& echo $$MESSAGETEXT | sed 's/./=/g' && echo $$MESSAGETEXT	\
	&& echo $$MESSAGETEXT | sed 's/./=/g' || true
check-local: check-TEST_PYPROGS
