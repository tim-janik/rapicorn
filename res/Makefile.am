# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl
ZINTERN    = ../rcore/rapicorn-zintern
EXTRA_DIST =
CLEANFILES =
noinst_DATA =

# == Images ==
RES_IMAGES = $(strip 		\
	image-broken.svg	\
)
OTHER = $(strip			\
)

# == Resource Pairs (Name, File) ==
RES_PAIRS   = $(foreach file, $(RES_IMAGES), icons/$(notdir $(file)) $(file))
RES_FILES   = $(RES_IMAGES)
EXTRA_DIST += $(RES_FILES) $(OTHER)

# == Rules ==
resources.cc: $(RES_FILES) $(ZINTERN) Makefile.am
	$(ZINTERN) -r $(RES_PAIRS) >xgen-$(@F)
	mv xgen-$(@F) $@
EXTRA_DIST += resources.cc
CLEANFILES += resources.cc
noinst_DATA += resources.cc
