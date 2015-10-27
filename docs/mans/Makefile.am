# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

man1_sources = rapidres.1.md rapidrun.1.md
EXTRA_DIST  += $(man1_sources)

# Find the latest revision (authoring) date for a file tracked in git
FILE_REVISION = $$(git log --format=%ai -n 99 -- $(1) | sort -rn | sed -n 's/ .*//; p; q')
# Substitute variables used in manual pages, use extra quotes to prevent Makefile.in substitutions
SED_MANINPUT  = "s/@""RAPICORN_VERSION@/Rapicorn-$(RAPICORN_VERSION)/; s/@""FILE_REVISION@/$(call FILE_REVISION, $(<))/" $<

# == Manual Pages man1 ==
man1_stems = $(man1_sources:%.1.md=%)
man1_MANS  = $(man1_stems:%=%-@MAJOR@.1)
%-@MAJOR@.1: @INGIT@ %.1.md
	$(AM_V_GEN)
	$(Q) sed $(SED_MANINPUT) > xgen-$(<F)
	$(Q) $(PANDOC) -s -t man xgen-$(<F) -o $@
	$(Q) rm -f xgen-$(<F)
man1-install-hook:
	$(Q) cd $(DESTDIR)$(man1dir) && \
	  for page in $(man1_stems) ; do \
	    $(call SYMLINK_LATEST, $$page-[0-9][0-9].1, $$page.1) && \
	  test -r $$page.1 -a -L $$page.1 || exit $$? ; done
install-data-hook: man1-install-hook
man1-uninstall-hook:
	$(Q) cd $(DESTDIR)$(man1dir) && \
	  for page in $(man1_stems) ; do \
	    $(call SYMLINK_LATEST, $$page-[0-9][0-9].1, $$page.1) && \
	  test -r $$page.1 -o ! -e $$page.1 || exit $$? ; done
uninstall-hook: man1-uninstall-hook
CLEANFILES += @INGIT@ $(man1_MANS)
EXTRA_DIST += $(man1_MANS)

# HTML manual pages to build, installation done by ../
noinst_DATA = $(man1_sources:%.1.md=%.1.html)
BODY_HEADER = $$'<a href="index.html">Return to Index</a><hr>\n'
BODY_FOOTER = $$'<hr>\n'
%.1.html: @INGIT@ %.1.md
	$(AM_V_GEN)
	$(Q) sed $(SED_MANINPUT) > xgen-$(<F)
	$(Q) echo ${BODY_HEADER} >xgen-bheader && echo ${BODY_FOOTER} > xgen-bfooter
	$(Q) $(PANDOC) -s -t html -S --toc -H head_style.css -A xgen-bfooter -B xgen-bheader xgen-$(<F) -o $@
	$(Q) rm -f xgen-bheader xgen-bfooter xgen-$(<F)
CLEANFILES += @INGIT@ $(noinst_DATA)
EXTRA_DIST += $(noinst_DATA) head_style.css
