# Rapicorn
# Copyright (C) 2007 Tim Janik
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

HTML_TARGETS = $(strip                  \
	html/				\
	html/main.html			\
	html/cmpgtk.html		\
	html/basics.html		\
	html/techbits.html		\
	html/rapicorn16.png		\
	html/rapicorn032.png		\
	html/hgradientG.png		\
	html/vgradientG.png		\
	html/style.css			\
	html/doxer-style.css		\
)
TARGET_DIRS = html/
DOXIDEFS = -D RAPICORN_VERSION ${RAPICORN_VERSION} -I $(srcdir)
DOCFRAME_DEFS=

# rule: mkdir
${TARGET_DIRS}:
	mkdir -p $@
# rule: .doxi -> .html
html/%.html: ./%.doxi
	$(DOXER) doxi2html $(strip $< -d html/  -D TOP_WEBDIR "." $(DOXIDEFS) $(call DOCFRAME_DEFS))
# rule: .png -> html/
html/%.png: ./%.png
	cp $< $@
# rule: .css -> html/
html/%.css: ./%.css
	cp $< $@
# rule: doxer-style.css
html/doxer-style.css:
	$(DOXER) -d html/ writecss
# rule: *.tar.bz2
rapicorn-site.tar.bz2: ${TARGET_DIRS} ${HTML_TARGETS}
	tar jcf $@ --anchored --exclude='html/screenshotdir/*' --exclude='html/logogallerydir/*' html/
CLEANFILES += rapicorn-site.tar.bz2
# rule: site*
site: rapicorn-site.tar.bz2
.PHONY: site
noinst_DATA = @DOXRULE@ rapicorn-site.tar.bz2
# cleanup
clean-local:
	rm -f screenshotdir/*.THUMB.jpg
	rm -f ${intermediate_files} ${CLEANFILES}
	rm -fr html/screenshotdir/ html/web-images/ html/images/ html/style/
	rm -f html/* html/.htaccess
	rmdir html/ 2>/dev/null ; true

# remote installation
ACCOUNT_USER = $(subst tjlocal, timj, ${USER})
ACCOUNT=$(strip $(ACCOUNT_USER))@rapicorn.org
ACDEST=/web/rapicorn/
ACTMP=xgen-tmp
site-update: rapicorn-site.tar.bz2
	scp -p $< ${ACCOUNT}:${ACTMP}-$(<F)
	ssh ${ACCOUNT}                                  \
	 "set -ex "                                     \
	 "&& tar jxmpf ${ACTMP}-$(<F) -C ${ACDEST} "    \
	 "&& rm ${ACTMP}-$(<F)"
.PHONY: site-update
