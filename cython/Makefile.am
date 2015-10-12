# Rapicorn Cython
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_GUI_CFLAGS) $(PYTHON_CPPFLAGS)

AIDACC       = $(top_builddir)/aidacc/aidacc-intern
RAPICORNIDL  = $(srcdir)/../ui/interfaces.idl
RPYTHON      = PYTHONPATH="$(abs_builddir)/${objdir}:$$PYTHONPATH" $(PYTHON)
XMANIFOLD    = $(top_srcdir)/aidacc/xmanifold.py
# Loading newly built Python modules requires accesss to libtool objdir
objdir       = .libs

# Py Rapicorn - installation
# pythondir = ${prefix}/lib/pythonX.Y/site-packages
Rapicorndir                = ${pythondir}/Rapicorn_@MAJOR@_@MINOR@
Rapicorn_PYTHON            = __init__.py
Rapicorn_LTLIBRARIES	   = Rapicorn.la
# Pyx Rapicorn module
nodist_Rapicorn_la_SOURCES = Rapicorn.cc	# nodist_ forces build time generation
Rapicorn_la_CXXFLAGS	   = $(AM_CXXFLAGS) -Wno-redundant-decls -Wno-unused-function
Rapicorn_la_LDFLAGS	   = $(AM_LDFLAGS) -no-undefined -module -avoid-version
Rapicorn_la_LIBADD	   = $(top_builddir)/ui/librapicorn-@MAJOR@.@MINOR@.la
EXTRA_DIST	          += Rapicorn.pyx pyxxutils.hh pyxxutils.pyx rcore.pyx PyxxStub.py
CLEANFILES	          += Rapicorn.cc idlapi.pyx

Rapicorn.cc: Rapicorn.pyx pyxxutils.pyx rcore.pyx idlapi.pyx pyxxutils-xmany.hh
	@echo "  CYTHON  " $@
	$(Q) $(CYTHON) --cplus -Wextra -Werror --fast-fail -I. $(srcdir)/Rapicorn.pyx -o $@
idlapi.pyx: $(RAPICORNIDL) $(srcdir)/PyxxStub.py
	@echo "  AIDACC  " $@
	$(Q) ${AIDACC} -x $(srcdir)/PyxxStub $(RAPICORNIDL) -o $@ -G strip-path=$(abs_top_srcdir)/
$(srcdir)/*.cc: pyxxutils-xmany.hh
pyxxutils-xmany.hh: $(srcdir)/pyxxutils.hh $(XMANIFOLD)
	$(AM_V_GEN)
	$(Q) $(XMANIFOLD) -S $(srcdir)/pyxxutils.hh 18 >xtmp-$(@F) \
	&& mv xtmp-$(@F) $@
CLEANFILES += pyxxutils-xmany.hh

# install-hook to pick version
rapicornmodule-install-hook:	# runs after install-data
	$(Q) cd $(DESTDIR)$(Rapicorndir)/.. && \
	  $(call SYMLINK_LATEST, Rapicorn_[0-9][0-9]_[0-9][0-9]/Rapicorn.so, Rapicorn.so) && \
	  test -x Rapicorn.so -a -L Rapicorn.so
install-data-hook: rapicornmodule-install-hook
rapicornmodule-uninstall-hook: # runs after uninstall
	$(Q) cd $(DESTDIR)$(Rapicorndir)/.. && \
	  $(call SYMLINK_LATEST, Rapicorn_[0-9][0-9]_[0-9][0-9]/Rapicorn.so, Rapicorn.so) && \
	  test -x Rapicorn.so -o ! -e Rapicorn.so
uninstall-hook: rapicornmodule-uninstall-hook


# === Tests ===
TEST_PYPROGS = testing1.py
EXTRA_DIST  += $(TEST_PYPROGS)

# === TEST_PYPROGS ===
.PHONY: check-TEST_PYPROGS
check-TEST_PYPROGS:
	$(Q) \
	  for tst in $(TEST_PYPROGS) ; do					\
	    [ -f ./"$$tst" ] && rt=./"$$tst" || rt="$(srcdir)/$$tst" ;		\
	    $(X11_ENV) $(RPYTHON) $$rt && echo "PASS: $$tst" || exit $? ;	\
	  done
	@MESSAGETEXT="All $(words $(TEST_PYPROGS)) Python tests passed"	\
	&& [ 0 -lt $(words $(TEST_PYPROGS)) ]				\
	&& echo $$MESSAGETEXT | sed 's/./=/g' && echo $$MESSAGETEXT	\
	&& echo $$MESSAGETEXT | sed 's/./=/g' || true
check-local: check-TEST_PYPROGS

python-call-info:
	@echo 'PYTHONPATH="$$PYTHONPATH:${pythondir}" python -ic "import Rapicorn"'
	@echo '${RPYTHON} -ic "import Rapicorn"'
