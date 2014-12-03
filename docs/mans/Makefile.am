# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

# Manual pages to build and install
man1_MANS = rapidres.1 rapidrun.1
%.1: %.1.md
	$(AM_V_GEN)
	$(Q) $(PANDOC) -s -t man $< -o $@
CLEANFILES += $(man1_MANS)

# HTML manual pages to build, installation done by ../
noinst_DATA = $(man1_MANS:%=%.html)
BODY_HEADER = $$'<a href="index.html">Return to Index</a><hr>\n'
BODY_FOOTER = $$'<hr>\n'
%.1.html: %.1.md
	$(AM_V_GEN)
	$(Q) echo ${BODY_HEADER} >xgen-bheader && echo ${BODY_FOOTER} > xgen-bfooter
	$(Q) $(PANDOC) -s -t html -S --toc -H head_style.css -A xgen-bfooter -B xgen-bheader $< -o $@
	$(Q) rm -f xgen-bheader xgen-bfooter
CLEANFILES += $(noinst_DATA)
EXTRA_DIST = head_style.css
