# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir) $(RAPICORN_GUI_CFLAGS)
DEFS		+=

bin_PROGRAMS	 = $(BIN_PROGS)
BIN_PROGS	 =
LDADDS		 = $(top_builddir)/ui/librapicorn@RAPICORN_RELEASE@.la

# rapidrun
BIN_PROGS	+= rapidrun
rapidrun_SOURCES = rapidrun.cc
rapidrun_LDADD   = $(LDADDS)


# check some options
check-rapidrun-basics:
	$(Q) ./rapidrun --list $(srcdir)/minitest.xml | grep -q "RapicornTest::MiniButton" \
	; eval "$$TSTDIAGNOSE" "'Check sample definition for MiniButton'"
check-am: check-rapidrun-basics
