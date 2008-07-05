# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= $(RAPICORN_CFLAGS)

bin_PROGRAMS	 = $(BIN_PROGS)
BIN_PROGS	 =
LDADDS		 = $(top_builddir)/rapicorn/librapicorn@RAPICORN_RELEASE@.la $(RAPICORN_DEP_LIBS)

# rapidrun
BIN_PROGS	+= rapidrun
rapidrun_SOURCES = rapidrun.cc
rapidrun_LDADD   = $(LDADDS)


# check some options
check-rapidrun-basics:
	./rapidrun --list $(top_builddir)/rapicorn/foundation.xml | grep -q VSlider
check-am: check-rapidrun-basics
