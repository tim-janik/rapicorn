# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

SUBDIRS = imports tutorial

# == Extra Tarball Files ==
EXTRA_DIST += ChangeLog-2009

# == Clean Up ==
clean-local:
	[ ! -d dev/ ] || $(MAKE) clean -C dev/
