# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

INCLUDES	+= -I$(top_srcdir)
DEFS		+= $(RAPICORN_CFLAGS) -DRAPICORN_PRIVATE

noinst_PROGRAMS	 = $(BIN_PROGS)
BIN_PROGS	 =
LDADDS		 = $(top_builddir)/rapicorn/librapicorn@LT_RELEASE@.la $(RAPICORN_DEP_LIBS)

# rapidrun
#BIN_PROGS	+= rapidrun
#rapidrun_SOURCES = rapidrun.cc
#rapidrun_LDADD   = $(LDADDS)
