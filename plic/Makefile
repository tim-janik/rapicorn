# plic - Pluggable IDL Compiler
#
## GNU Lesser General Public License version 2 or any later version.

srcdir=.

all:	plic.py
	$(MAKE) $(AM_MAKEFLAGS) check

plic.py: plic.g
	yapps plic.g
	# sed 's/^from yapps import runtime$$//' -i $@
	chmod +x $@

check:	plic.py test.idl
	$(srcdir)/plic.py test.idl
