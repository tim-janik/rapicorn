# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl
ZINTERN     = ../rcore/rapicorn-zintern
noinst_DATA =

# == Resource Files ==
RES_BITMAPS = $(strip \
	wm-gears.png \
)
RES_IMAGES = $(strip 		\
	image-broken.svg	\
	rapicorn.svg		\
)
RES_ROOT = $(strip		\
	stock.ini		\
)
RES_FILES   = $(RES_IMAGES) $(RES_ROOT)
EXTRA_DIST += $(RES_FILES)

# == Resource Pairs (Name, File) ==
IMAGE_PAIRS = $(foreach file, $(RES_IMAGES), rapicorn/images/$(notdir $(file)) $(file))
ROOT_PAIRS  = $(foreach file, $(RES_ROOT), rapicorn/$(notdir $(file)) $(file))
RES_PAIRS   = $(foreach file, $(RES_BITMAPS), icons/$(notdir $(file)) $(file))
RES_FILES   = $(RES_BITMAPS)
EXTRA_DIST += $(RES_FILES)

# == Rules ==
noinst_DATA += resources.cc
CLEANFILES  += resources.cc
resources.cc: $(RES_FILES) $(ZINTERN) Makefile.am
	$(AM_V_GEN)
	$(Q) $(ZINTERN) -r $(ROOT_PAIRS)	 > xgen-$(@F)
	$(Q) $(ZINTERN) -r $(IMAGE_PAIRS)	>> xgen-$(@F)
	$(Q) $(ZINTERN) -r $(RES_PAIRS)		>> xgen-$(@F)
	$(Q) mv xgen-$(@F) $@
