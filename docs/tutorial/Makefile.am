# Rapicorn
include $(top_srcdir)/Makefile.decl

PY_FILES	= tuthello.py

docdir        	= ${datarootdir}/doc/rapicorn@RAPICORN_RELEASE@
tutorialdir   	= ${docdir}/tutorial
tutorial_DATA	= $(PY_FILES)
EXTRA_DIST     += $(PY_FILES)

RPYTHON         = PYTHONPATH="$$PYTHONPATH:$(pythondir)" $(PYTHON)

# check tuthello.py by disabling the tutorials main loop and then executing its test code
check-tuthello.py:
	$(Q) sed '1,/^app\.loop()/s/^\(app\.loop()\)/#\1/' < tuthello.py > xtst-tuthello.py
	$(Q) $(RPYTHON) xtst-tuthello.py --test-123 ; test $$? -eq 123 ; eval "$$TSTDIAGNOSE"
	$(Q) rm -f xtst-tuthello.py
check-local: check-tuthello.py
