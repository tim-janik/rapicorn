# Rapicorn                      - non-recursive Makefile

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)

# non-migrated subdirs
SUBDIRS     = po . rcore aidacc ui cython tools tests examples docs

# == res/ ==
include res/Makefile.sub
