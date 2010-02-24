# Rapicorn Rope
include $(top_srcdir)/Makefile.decl

SUBDIRS = . # tests

PLICINT = $(top_builddir)/plic/plic-intern

EXTRA_DIST += base.idl

# tests
check-base-idl:
	@$(call TSTTITLE)
	$(_@) ${PLICINT} $(srcdir)/base.idl > /dev/null ; $(call TSTRESULT)
check-local: check-base-idl
