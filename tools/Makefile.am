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
	$(Q) ./rapidrun --list $(top_srcdir)/ui/foundation.xml | grep -q VSlider \
	; eval "$$TSTDIAGNOSE" "'Simple rapidrun VSlider test'"
check-am: check-rapidrun-basics
