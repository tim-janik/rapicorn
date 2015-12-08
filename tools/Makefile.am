# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_GUI_CFLAGS)

bin_PROGRAMS	 = $(BIN_PROGS)
BIN_PROGS	 =
LDADDS		 = $(top_builddir)/ui/librapicorn-@MAJOR@.la

# == rapidrun ==
RAPIDRUN                 = rapidrun # rapidrun-@MAJOR@
BIN_PROGS	        += $(RAPIDRUN)
rapidrun_SOURCES = rapidrun.cc
rapidrun_LDADD   = $(LDADDS)

# check some options
check-rapidrun-basics: $(RAPIDRUN)
	$(Q) ./$(RAPIDRUN) --list $(srcdir)/minitest.xml | grep -q "MiniButton" \
	; eval "$$TSTDIAGNOSE" "'Check sample definition for MiniButton'"
check-local: check-rapidrun-basics
EXTRA_DIST += minitest.xml
