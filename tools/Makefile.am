# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_GUI_CFLAGS)

bin_PROGRAMS	 = $(BIN_PROGS)
BIN_PROGS	 =
LDADDS		 = $(top_builddir)/ui/librapicorn-@MAJOR@.la

# == rapidrun ==
RAPIDRUNV                = rapidrun-@MAJOR@
BIN_PROGS	        += $(RAPIDRUNV)
rapidrun_@MAJOR@_SOURCES = rapidrun.cc
rapidrun_@MAJOR@_LDADD   = $(LDADDS)
rapidrun-install-hook:	 # run after ${RAPIDRUNV} was installed
	$(Q) cd $(DESTDIR)$(bindir) && \
	  $(call SYMLINK_LATEST, rapidrun-[0-9][0-9], rapidrun) && \
          test -x rapidrun -a -L rapidrun
install-exec-hook: rapidrun-install-hook
rapidrun-uninstall-hook: # run after ${RAPIDRUNV} was uninstalled
	$(Q) cd $(DESTDIR)$(bindir) && \
	  $(call SYMLINK_LATEST, rapidrun-[0-9][0-9], rapidrun) && \
	  test -x rapidrun -o ! -e rapidrun
uninstall-hook: rapidrun-uninstall-hook

# check some options
check-rapidrun-basics: $(RAPIDRUNV)
	$(Q) ./$(RAPIDRUNV) --list $(srcdir)/minitest.xml | grep -q "MiniButton" \
	; eval "$$TSTDIAGNOSE" "'Check sample definition for MiniButton'"
check-am: check-rapidrun-basics
EXTRA_DIST += minitest.xml
