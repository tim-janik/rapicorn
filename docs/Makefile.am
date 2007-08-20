# Rapicorn
# Copyright (C) 2007 Tim Janik
#
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

# extract source code documentation as psrc
PSOURCE_TARGETS = $(strip			\
	psource/rapicorn.psrc			\
)
# noinst_DATA = ${PSOURCE_TARGETS}

DOXIDEFS = -D RAPICORN_VERSION ${RAPICORN_VERSION} -I $(srcdir) -D TOP_WEBDIR "."

# Reference Documentation
make_psource = $(strip $(DOXER) src2pickle -o $(1) $(DOXIDEFS) $(2))
# RAPICORN reference docu
rapicorn_rdocu_globs   = $(top_srcdir)/rapicorn/*.[hc] $(top_srcdir)/rapicorn/*.[hc][hc]
rapicorn_rdocu_exclude = rapicornconfig.h
rapicorn_rdocu_files   = $(filter-out $(foreach f, $(rapicorn_rdocu_exclude), $(wildcard $(top_srcdir)/rapicorn/$(f))), $(wildcard $(rapicorn_rdocu_globs)))
psource/rapicorn.psrc: $(rapicorn_rdocu_files)
	@mkdir -p psource/
	$(call make_psource, $@, $(filter %.h %.hh, $(rapicorn_rdocu_files)) -Y $(filter-out %.h %.hh, $(rapicorn_rdocu_files)) )
# clean up
clean-local:
	rm -rf psource/
