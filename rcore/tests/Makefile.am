# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE
AM_CXXFLAGS += $(RAPICORN_CORE_CFLAGS) $(SVG_DEPENDENCIES_CFLAGS)

# test programs
noinst_PROGRAMS	 = $(TAPTESTS)
LDADDS		 = $(top_builddir)/rcore/librapicorncore-@MAJOR@.la $(SVG_DEPENDENCIES_LIBS)

# === test rules ===
# check startup entropy
check-startup-entropy:
	$(Q) A=`./benchrcore --entropy` ; B=`./benchrcore --entropy`; \
	     test -n "$$A$$B" -a "$$A" != "$$B"; $(TSTRESULT)
check-system-entropy:
	$(Q) A=`./benchrcore --system-entropy` ; B=`./benchrcore --system-entropy`; \
	     test -n "$$A$$B" -a "$$A" != "$$B"; $(TSTRESULT)
check-runtime-entropy:
	$(Q) A=`./benchrcore --runtime-entropy` ; B=`./benchrcore --runtime-entropy`; \
	     test -n "$$A$$B" -a "$$A" != "$$B"; $(TSTRESULT)
check-local: check-startup-entropy check-system-entropy check-runtime-entropy
