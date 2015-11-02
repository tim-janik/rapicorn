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
man1_MANS  = $(man1_stems:%=%.1) # $(man1_stems:%=%-@MAJOR@.1)
%.1: @INGIT@ %.1.md
	$(AM_V_GEN)
	$(Q) sed $(SED_MANINPUT) > xgen-$(@F)
	$(Q) $(PANDOC) -s -t man xgen-$(@F) -o $@
	$(Q) rm -f xgen-$(@F)
CLEANFILES += @INGIT@ $(man1_MANS)
EXTRA_DIST += $(man1_MANS)

# HTML manual pages to build, installation done by ../
noinst_DATA = $(man1_sources:%.1.md=%.1.html)
BODY_HEADER = $$'<a href="index.html">Return to Index</a><hr>\n'
BODY_FOOTER = $$'<hr>\n'
%.1.html: @INGIT@ %.1.md
	$(AM_V_GEN)
	$(Q) sed $(SED_MANINPUT) > xgen-$(@F)
	$(Q) echo ${BODY_HEADER} > xgen-B-$(@F) && echo ${BODY_FOOTER} > xgen-A-$(@F)
	$(Q) $(PANDOC) -s -t html -S --toc -H head_style.css -A xgen-A-$(@F) -B xgen-B-$(@F) xgen-$(@F) -o $@
	$(Q) rm -f xgen-B-$(@F) xgen-A-$(@F) xgen-$(@F)
CLEANFILES += @INGIT@ $(noinst_DATA)
EXTRA_DIST += $(noinst_DATA) head_style.css
