# Rapicorn
include $(top_srcdir)/Makefile.decl

PY_FILES	= tuthello.py

docdir        	= ${datarootdir}/doc/rapicorn-@MAJOR@
tutorialdir   	= ${docdir}/tutorial
tutorial_DATA	= $(PY_FILES)
EXTRA_DIST     += $(PY_FILES)

FPYTHON         = PYTHONPATH="$$PYTHONPATH:$(pythondir)" $(PYTHON)

# check tuthello.py by disabling the tutorials main loop and then executing its test code
check-tuthello.py: 
	$(Q) sed '1,/^app\.run()/s/^\(app\.run()\)/#\1/' < $(srcdir)/tuthello.py > xtst-tuthello.py
	$(Q) $(X11_ENV) $(FPYTHON) xtst-tuthello.py --test-123 ; test $$? -eq 123 ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f xtst-tuthello.py
installcheck-local: check-tuthello.py
