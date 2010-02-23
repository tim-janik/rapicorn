# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
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
	@$(call TSTTITLE, Simple rapidrun VSlider test)
	$(_@) ./rapidrun --list $(top_srcdir)/ui/foundation.xml | grep -q VSlider; \
	  $(call TSTRESULT)
check-am: check-rapidrun-basics
