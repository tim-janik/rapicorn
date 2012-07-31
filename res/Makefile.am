# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl
ZINTERN    = ../rcore/rapicorn-zintern
EXTRA_DIST =
CLEANFILES =
noinst_DATA =

# == Bitmaps ==
RES_BITMAPS = $(strip \
)

# == Resource Pairs (Name, File) ==
RES_PAIRS   = $(foreach file, $(RES_BITMAPS), icons/$(notdir $(file)) $(file))
RES_FILES   = $(RES_BITMAPS)
EXTRA_DIST += $(RES_FILES)

# == Rules ==
resources.cc: $(RES_FILES) $(ZINTERN) Makefile.am
	$(ZINTERN) -r $(RES_PAIRS) >xgen-$(@F)
	mv xgen-$(@F) $@
EXTRA_DIST += resources.cc
CLEANFILES += resources.cc
noinst_DATA += resources.cc
