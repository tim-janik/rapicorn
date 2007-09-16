# Rapicorn
# Copyright (C) 2007 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

# extract source code documentation as psrc
PSOURCE_TARGETS = $(strip			\
	psource/rapicorn-core.psrc		\
	psource/rapicorn-gui.psrc		\
)
# noinst_DATA = ${PSOURCE_TARGETS}

DOXIDEFS = -D RAPICORN_VERSION ${RAPICORN_VERSION} -I $(srcdir) -D TOP_WEBDIR "."

# Reference Documentation
make_psource = $(strip $(DOXER) src2pickle -o $(1) $(DOXIDEFS) $(2))
# RAPICORN reference docu
rapicorn_core_globs   = $(top_srcdir)/rapicorn-core/*.[hc] $(top_srcdir)/rapicorn-core/*.[hc][hc]
rapicorn_core_exclude = rapicornconfig.h
rapicorn_core_files   = $(filter-out $(foreach f, $(rapicorn_core_exclude), $(wildcard $(top_srcdir)/rapicorn-core/$(f))), $(wildcard $(rapicorn_core_globs)))
rapicorn_gui_globs    = $(top_srcdir)/rapicorn/*.[hc] $(top_srcdir)/rapicorn/*.[hc][hc]
rapicorn_gui_exclude  = rapicornconfig.h
rapicorn_gui_files    = $(filter-out $(foreach f, $(rapicorn_gui_exclude), $(wildcard $(top_srcdir)/rapicorn/$(f))), $(wildcard $(rapicorn_gui_globs)))
psource/rapicorn-core.psrc: $(rapicorn_core_files)
	@mkdir -p psource/
	$(call make_psource, $@, $(filter %.h %.hh, $(rapicorn_core_files)) -Y $(filter-out %.h %.hh, $(rapicorn_core_files)) )
psource/rapicorn-gui.psrc: $(rapicorn_gui_files)
	@mkdir -p psource/
	$(call make_psource, $@, $(filter %.h %.hh, $(rapicorn_gui_files)) -Y $(filter-out %.h %.hh, $(rapicorn_gui_files)) )

# clean up
clean-local:
	rm -rf psource/
