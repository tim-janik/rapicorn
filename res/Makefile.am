# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl
ZINTERN     = ../rcore/rapicorn-zintern
noinst_DATA =

# == Resource Files ==
ROOT_FILES = $(strip		\
	stock.ini		\
)
BITMAP_FILES = $(strip 		\
	wm-gears.png		\
)
IMAGE_FILES = $(strip 		\
	image-broken.svg	\
	rapicorn.svg		\
)

# == Resource Pairs (Name, File) ==
RES_FILES    =
ROOT_PAIRS   = $(foreach file, $(ROOT_FILES), rapicorn/$(notdir $(file)) $(srcdir)/$(file))
RES_FILES   += $(ROOT_FILES)
BITMAP_PAIRS = $(foreach file, $(BITMAP_FILES), icons/$(notdir $(file)) $(srcdir)/$(file))
RES_FILES   += $(BITMAP_FILES)
IMAGE_PAIRS  = $(foreach file, $(IMAGE_FILES), rapicorn/images/$(notdir $(file)) $(srcdir)/$(file))
RES_FILES   += $(IMAGE_FILES)
EXTRA_DIST  += $(RES_FILES)

# == Rules ==
noinst_DATA += resources.cc
CLEANFILES  += resources.cc
resources.cc: $(RES_FILES) $(ZINTERN) Makefile.am
	$(AM_V_GEN)
	$(Q) $(ZINTERN) -r $(ROOT_PAIRS)	 > xgen-$(@F)
	$(Q) $(ZINTERN) -r $(BITMAP_PAIRS)	>> xgen-$(@F)
	$(Q) $(ZINTERN) -r $(IMAGE_PAIRS)	>> xgen-$(@F)
	$(Q) mv xgen-$(@F) $@
