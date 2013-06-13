# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

HTMLMANS    = Rapidrun.1.html
EXTRA_DIST += $(HTMLMANS)

import: check-tools
	$(AM_V_GEN)
	$(Q) rm -f $(HTMLMANS)
	$(Q) $(MAKE) $(AM_MAKEFLAGS) --no-print-directory $(HTMLMANS)
.PHONY: import

# = Check Tools =
CHECK_WGET = { command -v wget >/dev/null && { echo "1.10.0" ; wget --version 2>&1 | sed 's/[^0-9]*// ; 1q' ; } | sort -VC ; }
check-tools:
	$(Q) ${CHECK_WGET} || { echo "$@: failed to detect recent version: wget"; exit 1; }

# = HTMLMANS =
HEADER= $$'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">\n<html>\n' \
	$$'<head>\n' \
	$$'<title>'"$$TITLE"$$'</title>\n' \
	$$'<style type="text/css">\n' \
	$$'body { font-family: "DejaVu Sans", "Bitstream Vera Sans", Corbel, Verdana, "Verdana Ref", sans-serif; }\n' \
	$$'.tocnumber { display: none; }\n' \
	$$'body > p, body > dl, body > ul, body > ol { margin-left: 8em; }\n' \
	$$'</style></head>\n<body>'
FOOTER= $$'</body>\n</html>'
STEMTMP = xtmp-$(*F)
%.1.html:
	$(AM_V_GEN)
	$(Q) rm -rf $(STEMTMP)
	$(Q) mkdir $(STEMTMP)
	$(Q) : # wget -k needs the final target name
	$(Q) cd $(STEMTMP) && wget -nv -k --no-cache "http://testbit.eu/$(@F:.html=)?action=render" -O $(@F)
	$(Q) TITLE="$(@F:.html=)" && echo ${HEADER} > $(STEMTMP)/header
	$(Q) echo ${FOOTER} > $(STEMTMP)/footer
	$(Q) cat $(STEMTMP)/header $(STEMTMP)/$(@F) $(STEMTMP)/footer > $(STEMTMP)/combined
	$(Q) mv $(STEMTMP)/combined $@
	$(Q) rm -rf $(STEMTMP)
