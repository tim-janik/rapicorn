# Rapicorn                      - non-recursive Makefile

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/Makefile.decl
topdir = $(abs_top_srcdir)
V_gen = @printf '  %-9s%s\n'

# == non-migrated subdirs ==
SUBDIRS		 = po .

# == make setup ==
DEFS			+= -DRAPICORN_CONVENIENCE -D__RAPICORN_BUILD__ -D__FILE_DIR__="\"rapicorn\"" -D__TOPDIR__="\"$(topdir)/\""
AM_CPPFLAGS		+= -I$(top_builddir) # -I$(top_srcdir)
AM_CXXFLAGS		+=
lib_LTLIBRARIES 	 =
bin_PROGRAMS		 =
bin_SCRIPTS		 =
noinst_DATA		 =
noinst_SCRIPTS		 =
noinst_PROGRAMS 	 =
man1_MANS		 =
man1_HTMLS		 =
make_check_pytests	 =
make_check_xml_files	 =
make_check_targets	 =
revisioned_files	 = # files that need @FILE_REVISION@ substitution

# == project tools and paths ==
# objdir:		# Loading newly built Python modules requires accesss to libtool objdir
objdir			= .libs
TAPTOOL 		= $(topdir)/misc/taptool.sh
XMANIFOLD		= $(topdir)/misc/xmanifold.py
YAPPS			= $(PYTHON) $(topdir)/yapps2_deb/yapps2.py
AIDACC_INTERN		= $(top_builddir)/aidacc/aidacc-intern
AIDACC_CXXSTUB_PY	= aidacc/CxxStub.py
WITH_PYTHONPATH_INTERN	= PYTHONPATH="$(abs_top_builddir)/cython/$(objdir):$$PYTHONPATH"
PYTHON_INTERN		= $(WITH_PYTHONPATH_INTERN) $(PYTHON)
XVFB_PYTHON_INTERN	= $(WITH_PYTHONPATH_INTERN) $(X11_ENV) $(PYTHON)
XVFB_RAPIDRUN		= $(X11_ENV) $(abs_top_builddir)/tools/rapidrun --fatal-warnings "$$@"
IMGCHECK		= tools/imgcheck

# == res/ ==
include res/Makefile.sub

# == aidacc/ ==
include aidacc/Makefile.sub

# == rcore/ ==
include rcore/Makefile.sub

# == ui/ ==
include ui/Makefile.sub

# == cython/ ==
include cython/Makefile.sub

# == tools/ ==
include tools/Makefile.sub

# == docs/ ==
include docs/Makefile.sub

# == tests/ ==
include tests/Makefile.sub

# == examples/ ==
include examples/Makefile.sub

# == revisions.lst ==
noinst_DATA += revisions.lst
EXTRA_DIST  += revisions.lst $(revisioned_files)
revisions.lst: @INGIT@ $(revisioned_files) Makefile
	$(AM_V_GEN)
	$(Q) rm -f $@.tmplst
	$(Q) for file in $(revisioned_files) ; do \
	  git log --format="%ai $$file" -n 99 -- $$file > $@.tmp1 && \
	  sort -rn $@.tmp1 > $@.tmp2 && \
	  sed -n '1{ p; q}' $@.tmp2 >> $@.tmplst || exit $$? ; \
	done
	$(Q) mv $@.tmplst $(topdir)/$(@F)
	$(Q) rm -f $@.tmp1 $@.tmp2 $@.tmplst
clean-revisions.lst: # git is required to rebuild revisions.lst, so conditionalize cleaning
	! git rev-parse --git-dir 2>/dev/null 1>&2 || rm -f $(topdir)/revisions.lst
.PHONY: clean-revisions.lst
clean-local: clean-revisions.lst

# == .revd.md (INTERMEDIATE) ==
%.revd.md: %.md revisions.lst
	$(AM_V_GEN)
	$(Q) V=`sed -n '\| $<$$|{ s/ .*//; p; q; }' $(topdir)/revisions.lst` \
	&& sed "s/[@]FILE_REVISION[@]/$$V/g; s/[@]BUILDID[@]/$(topbuildid)/g" < $< > $@

# == man.1 (markdown) ==
%.1: %.1.revd.md
	$(AM_V_GEN)
	$(Q) $(PANDOC) -s -t man $< -o $@

# == man.1.html (markdown) ==
manpage_header.html:
	$(Q) echo $$'<a href="../index.html">Return to Index</a>\n<hr>\n' > $@
.INTERMEDIATE: manpage_header.html
CLEANFILES += manpage_header.html
manpage_footer.html:
	$(Q) echo $$'<hr>\n<a href="../index.html">Return to Index</a>\n' > $@
.INTERMEDIATE: manpage_footer.html
CLEANFILES += manpage_footer.html
%.1.html: %.1.revd.md manpage_header.html manpage_footer.html $(topdir)/docs/manpage.css
	$(AM_V_GEN)
	$(Q) $(PANDOC) -s -t html -S --toc -H $(topdir)/docs/manpage.css -B manpage_header.html -A manpage_footer.html $< -o $@

# == topbuildid.cc ==
topbuildid ::= $(shell $(topdir)/misc/mkbuildid.sh -p topbuildid.cc)	# "update rule" for topbuildid.cc
topbuildid.cc: # avoid explicit deps, 'topbuildid::=' ensures updates
CLEANFILES += topbuildid.cc
buildid:
	@echo $(topbuildid)

# == make_check_targets ==
.PHONY: $(make_check_targets)
check-local: $(make_check_targets)

# == stamp-autocheck-xmllint ==
EXTRA_DIST	+= $(make_check_xml_files)
noinst_DATA	+= stamp-autocheck-xmllint
CLEANFILES	+= stamp-autocheck-xmllint
stamp-autocheck-xmllint: $(make_check_xml_files)
	$(Q) for XMLFILE in $? ; do \
	  $(XMLLINT) --noout "$$XMLFILE" ; eval "$$TSTDIAGNOSE" "$$XMLFILE" || exit $$? ; \
	done
	$(Q) touch $@

# == python-call-info ==
python-call-info:
	@echo '$(PYTHON_INTERN) -ic "import Rapicorn"'
	@echo 'PYTHONPATH="'$$(readlink -f $(DESTDIR)$(cython_pymodulesdir)/..)':$$PYTHONPATH" $(PYTHON) -ic "import Rapicorn"'
.PHONY: python-call-info

# == pkg-config-call-info ==
rapicorn_pkgconfigdir    = $(libdir)/pkgconfig
pkg-config-call-info:
	@echo 'PKG_CONFIG_PATH="'$(DESTDIR)$(rapicorn_pkgconfigdir):$$PKG_CONFIG_PATH'" $(PKG_CONFIG)'
.PHONY: pkg-config-call-info

# == check-python-tests ==
check-python-tests:
EXTRA_DIST += $(make_check_pytests)
define make_check_pytests_TEMPLATE
$(1)--test: $(1)
	$$(Q) $$(XVFB_PYTHON_INTERN) $(1) ; eval "$$$$TSTDIAGNOSE" $(1)
.PHONY: $(1)--test
check-python-tests: $(1)--test
endef
$(foreach PYTEST, $(make_check_pytests), $(eval $(call make_check_pytests_TEMPLATE, $(PYTEST))))
.PHONY: check-python-tests
check-local: check-python-tests

# == ./ dependencies ==
rapicorn.hh: ui/clientapi.hh
$(SOURCES): rapicorn.hh

# == include/ install ==
rapicorn_includedir      = $(includedir)/rapicorn-@MAJOR@/
rapicorn_include_HEADERS = rapicorn-core.hh rapicorn-test.hh rapicorn.hh
EXTRA_DIST		+= $(rapicorn_include_HEADERS)

# == rapicorn.pc ==
rapicorn_pkgconfig_DATA  = rapicorn-@MAJOR@.pc
EXTRA_DIST		+= rapicorn.pc.in
rapicorn-@MAJOR@.pc: rapicorn.pc.in $(top_builddir)/config.status
	$(AM_V_GEN)
	$(Q) cd $(top_builddir) && $(SHELL) ./config.status --file=$@:$<
CLEANFILES += rapicorn-@MAJOR@.pc
# install rapicorn.pc -> rapicorn-@MAJOR@.pc symlink
rapicorn-pc-install-hook:	# runs after install-data
	$(Q) cd $(DESTDIR)$(rapicorn_pkgconfigdir) && \
	  $(call SYMLINK_LATEST, rapicorn-[0-9][0-9].pc, rapicorn.pc) && \
	  test -r rapicorn.pc -a -L rapicorn.pc
install-data-hook: rapicorn-pc-install-hook
rapicorn-pc-uninstall-hook:	# runs after uninstall
	$(Q) cd $(DESTDIR)$(rapicorn_pkgconfigdir) && \
	  $(call SYMLINK_LATEST, rapicorn-[0-9][0-9].pc, rapicorn.pc) && \
	  test -r rapicorn.pc -o ! -e rapicorn.pc
uninstall-hook: rapicorn-pc-uninstall-hook

# == doc/ install ==
projectdocsdir   = $(docdir)/
projectdocs_DATA = README NEWS COPYING.MPL

# == README ==
README: README.md
	$(AM_V_GEN)
	$(Q) rm -f $@		# work around read-only target
	$(Q) $(PANDOC) -S -f markdown_github+pandoc_title_block-hard_line_breaks -t plain $< -o $@
	$(Q) chmod a-w $@	# prevent accidental target file edits
.SECONDARY: README.md		# pacify make(1) when shipping README without README.md
MAINTAINERCLEANFILES += README

# == cleanups ==
CLEANFILES +=			\
	yapps2_deb/yapps/*.pyc
MAINTAINERCLEANFILES += 	\
	Makefile\.in		\
	config/compile		\
	aclocal\.m4		\
	autogen\.log		\
	configure		\
	configure\.h\.in	\
	configure\.h\.in~	\
	config/config\.guess	\
	config/config\.sub	\
	config/depcomp		\
	config/install-sh	\
	config/intltool\.m4	\
	config/libtool\.m4	\
	config/ltmain\.sh	\
	config/ltoptions\.m4	\
	config/ltsugar\.m4	\
	config/ltversion\.m4	\
	config/lt~obsolete\.m4	\
	config/missing		\
	config/py-compile
