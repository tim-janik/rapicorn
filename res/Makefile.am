# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl
ZINTERN     = ../rcore/rapicorn-zintern
noinst_DATA =

# == Makefile.res ==
-include Makefile.res	# define RES_FILE_LIST
streq = $(and $(findstring $(1),$(2)),$(findstring $(2),$(1)))
NEW_FILE_LIST = $(filter-out Makefile%, $(wildcard */* *))
Makefile.res: @INGIT@ $(if $(call streq, $(NEW_FILE_LIST), $(RES_FILE_LIST)),, Makefile.tmp)
	$(AM_V_GEN)
	@mv Makefile.tmp $@
Makefile.tmp: # MAKE(1) only remakes+rereads Makefiles with prerequisites (Remaking Makefiles)
	@echo "RES_FILE_LIST = $(NEW_FILE_LIST)" > $@
noinst_DATA = Makefile.res

# == Ship Resource Files ==
EXTRA_DIST = $(RES_FILE_LIST)
