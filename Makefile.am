# Rapicorn
# Copyright (C) 2005 Tim Janik

SUBDIRS = . rapicorn po examples tests

# require automake 1.4
AUTOMAKE_OPTIONS = 1.4

test:
	$(MAKE) -C tests/ $(AM_MAKEFLAGS) $@
.PHONY: test

# extra dependancies
configure: acbirnet.m4

CLEANFILES += $(strip		\
	intltool-extract	\
	intltool-merge		\
	intltool-update		\
)

EXTRA_DIST += $(strip		\
	TODO			\
	COPYING.GPL		\
	COPYING.LGPL		\
	acbirnet.m4		\
 	po-helper.sh.in		\
	intltool-extract.in     \
	intltool-merge.in       \
	intltool-update.in	\
)
