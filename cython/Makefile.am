# Rapicorn Cython
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_GUI_CFLAGS) $(PYTHON_CPPFLAGS)

AIDACC       = $(top_builddir)/aidacc/aidacc-intern
RAPICORNIDL  = $(srcdir)/../ui/interfaces.idl
XMANIFOLD    = $(top_srcdir)/aidacc/xmanifold.py
# Loading newly built Python modules requires accesss to libtool objdir
objdir       = .libs
# call python with uninstalled or fully installed modules
UPYTHON      = PYTHONPATH="$(abs_builddir)/${objdir}:$$PYTHONPATH" $(PYTHON)
FPYTHON      = PYTHONPATH="$$PYTHONPATH:${pythondir}" $(PYTHON)

# Py Rapicorn - installation
# pythondir = ${prefix}/lib/pythonX.Y/site-packages
Rapicorndir                = ${pythondir}/Rapicorn_@MAJOR@
Rapicorn_LTLIBRARIES	   = Rapicorn.la
# Pyx Rapicorn module
nodist_Rapicorn_la_SOURCES = Rapicorn.cc	# nodist_ forces build time generation
Rapicorn_la_CXXFLAGS	   = $(AM_CXXFLAGS) -Wno-redundant-decls -Wno-unused-function
Rapicorn_la_LDFLAGS	   = $(AM_LDFLAGS) -no-undefined -module -avoid-version
Rapicorn_la_LIBADD	   = $(top_builddir)/ui/librapicorn-@MAJOR@.la
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

# install-hook for __init__.py
# cython gets confused about having an __init__.py in builddir, so we
# keep it around under a different name.
rapicornmodule-install-initpy:
	$(INSTALL_DATA) __init__.py.in "$(DESTDIR)$(Rapicorndir)"/__init__.py
	$(am__py_compile) --destdir "$(DESTDIR)" --basedir "$(Rapicorndir)" __init__.py
install-data-hook: rapicornmodule-install-initpy
rapicornmodule-uninstall-initpy: # see PEP-3147 for __pycache__
	@RMPY=__init__.py && \
	RMPYC=`echo "$$RMPY" | sed 's/$$/c/'` && \
	RMPYO=`echo "$$RMPY" | sed 's/$$/o/'` && \
	RMCHC=`echo "$$RMPY" | sed -e 's|\.py$$|| ; s|[^/]*$$|__pycache__/&.*.pyc|'` && \
	RMCHO=`echo "$$RMPY" | sed -e 's|\.py$$|| ; s|[^/]*$$|__pycache__/&.*.pyo|'` && \
	PS4= && set -x && \
	cd "$(DESTDIR)$(Rapicorndir)" && \
	rm -f "$$RMPY" "$$RMPYC" "$$RMPYO" $$RMCHC $$RMCHO
uninstall-hook: rapicornmodule-uninstall-initpy
EXTRA_DIST += __init__.py.in

# install-hook to pick version
rapicornmodule-install-hook:	# runs after install-data
	$(Q) cd $(DESTDIR)$(Rapicorndir)/.. && \
	  $(call SYMLINK_LATEST, Rapicorn_[0-9][0-9]/Rapicorn.so, Rapicorn.so) && \
	  test -x Rapicorn.so -a -L Rapicorn.so
install-data-hook: rapicornmodule-install-hook
rapicornmodule-uninstall-hook: # runs after uninstall
	$(Q) cd $(DESTDIR)$(Rapicorndir)/.. && \
	  $(call SYMLINK_LATEST, Rapicorn_[0-9][0-9]/Rapicorn.so, Rapicorn.so) && \
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
	    $(X11_ENV) $(UPYTHON) $$rt && echo "PASS: $$tst" || exit $? ;	\
	  done
	@MESSAGETEXT="All $(words $(TEST_PYPROGS)) Python tests passed"	\
	&& [ 0 -lt $(words $(TEST_PYPROGS)) ]				\
	&& echo $$MESSAGETEXT | sed 's/./=/g' && echo $$MESSAGETEXT	\
	&& echo $$MESSAGETEXT | sed 's/./=/g' || true
check-local: check-TEST_PYPROGS

testing1-installcheck:
	@ # Patch testing1.py to use the installed versioned Rapicorn.so module
	$(Q) sed 's/.*@LINE_REWRITTEN_FOR_INSTALLCHECK@.*/import Rapicorn_@MAJOR@ as Rapicorn/' \
	       < $(srcdir)/testing1.py > x-testing1.py && ! cmp -s $(srcdir)/testing1.py x-testing1.py
	$(Q) $(X11_ENV) $(FPYTHON) x-testing1.py
	$(Q) rm -f x-testing1.py
installcheck-local: testing1-installcheck

python-call-info:
	@echo '${FPYTHON} -ic "import Rapicorn"'
	@echo '${UPYTHON} -ic "import Rapicorn"'
