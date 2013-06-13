# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

HTMLMANS    = Rapidrun.1.html
EXTRA_DIST += $(HTMLMANS)

import: $(HTMLMANS)
.PHONY: import

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
	rm -rf $(STEMTMP)
	mkdir $(STEMTMP)
	: # wget -k needs the final target name
	cd $(STEMTMP) &&	wget -k --no-cache "http://testbit.eu/$(@F:.html=)?action=render" -O $(@F)
	TITLE="$(@F:.html=)" && echo ${HEADER} > $(STEMTMP)/header
	echo ${FOOTER} > $(STEMTMP)/footer
	cat $(STEMTMP)/header $(STEMTMP)/$(@F) $(STEMTMP)/footer > $(STEMTMP)/combined
	mv $(STEMTMP)/combined $@
	rm -rf $(STEMTMP)
