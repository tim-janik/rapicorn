# Rapicorn
# Copyright (C) 2010 Tim Janik
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

DOX_FILES	= pyhello.dox
PY_TARGETS	= pyhello.py

docdir        	= ${datarootdir}/doc/rapicorn@RAPICORN_RELEASE@
tutorialdir   	= ${docdir}/tutorial
tutorial_DATA	= $(DOX_FILES) $(PY_TARGETS)
CLEANFILES  	= $(PY_TARGETS) xtmp-*
EXTRA_DIST  	= $(DOX_FILES)

# sed: enter @code, print empty newline, print and loop lines unil @endcode
EXTRACT_CODE_ENDCODE = sed -n '/@code/{ s/.*//; :MORE p; n; /@endcode/!bMORE; }'

%.py: %.dox
	$(AM_V_GEN)
	$(Q) $(EXTRACT_CODE_ENDCODE) < $< > xtmp-$(@F)
	$(Q) cp xtmp-$(@F) $@ && rm -f xtmp-$(@F)
