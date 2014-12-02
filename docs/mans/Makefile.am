# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

# Manual pages to build and install
man1_MANS = rapidres.1
%.1: %.1.md
	$(AM_V_GEN)
	$(Q) $(PANDOC) -s -t man $< -o $@
CLEANFILES += $(man1_MANS)

# HTML manual pages to build, installation done by ../
noinst_DATA = $(man1_MANS:%=%.html)
%.1.html: %.1.md
	$(AM_V_GEN)
	$(Q) $(PANDOC) -s --toc -t html -S --toc $< -o $@
CLEANFILES += $(noinst_DATA)
