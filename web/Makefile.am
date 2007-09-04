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
	html/roadmap.html		\
	html/versioning.html		\
	html/style.css			\
	html/doxer-style.css		\
)
RDOCU_TARGETS = $(strip                 \
	html/rdocu-rapicorn/index.html  \
)

TARGET_DIRS   = html/ tmpdoxi-rapicorn/ html/rdocu-rapicorn/
DOXIDEFS      = -D RAPICORN_VERSION ${RAPICORN_VERSION} -I $(srcdir)
DOCFRAME_DEFS = -D DOXI_INCLUDE_FILE \"definitions.doxi\"    # $(if $(findstring $(<F), ${WEBMENU_DOXI_FILES}), \"webmenu.doxi\", \"webframe.doxi\")

# Reference Documentation
../docs/psource/rapicorn.psrc: PHONY-TARGET
	@$(MAKE) -C ../docs $(patsubst ../docs/%, %, $@)
# Rapicorn reference docu
tmpdoxi-rapicorn/index.doxi: ../docs/psource/rapicorn.psrc rdocu-template.doxi definitions.doxi
	rm -f tmpdoxi-rapicorn/*
	$(DOXER) pickle2doxi $(strip -i $<      -D TOP_WEBDIR ".."              \
		 -d tmpdoxi-rapicorn/ $(DOXIDEFS) -T rdocu-template.doxi \
		 -D DOCU_TEMPLATE_TITLE "Rapicorn - Interface Reference"        \
		 -D DOCU_TEMPLATE_HEADING "Rapicorn Interface Reference" )
html/rdocu-rapicorn/index.html: tmpdoxi-rapicorn/index.doxi $(wildcard tmpdoxi-rapicorn/*.doxi)
	rm -f html/rdocu-rapicorn/*
	$(DOXER) doxi2html $(strip                      -D TOP_WEBDIR ".."      \
		 -d html/rdocu-rapicorn/ $(DOXIDEFS) $(call DOCFRAME_DEFS)      \
		 $(filter-out $<, $(shell echo tmpdoxi-rapicorn/*.doxi)) )
	$(DOXER) doxi2html $(strip                      -D TOP_WEBDIR ".."      \
		 -d html/rdocu-rapicorn/ $(DOXIDEFS) $(call DOCFRAME_DEFS) $< )
EXTRA_DIST += rdocu-template.doxi

# rule: mkdir
${TARGET_DIRS}:
	mkdir -p $@
# rule: .doxi -> .html
html/%.html: ./%.doxi definitions.doxi
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
rapicorn-site.tar.bz2: ${TARGET_DIRS} ${HTML_TARGETS} ${RDOCU_TARGETS}
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
	rm -f tmpdoxi-rapicorn/* html/rdocu-rapicorn/*
	-rmdir -p tmpdoxi-rapicorn/ html/rdocu-rapicorn/
	rm -fr html/screenshotdir/ html/web-images/ html/images/ html/style/
	rm -f html/* html/.htaccess
	rmdir html/ 2>/dev/null ; true
.PHONY: PHONY-TARGET

# preview installation
RAPICORN_PREVIEW=~/public_html/rapicorn-preview
site-preview: ${RAPICORN_PREVIEW}/. rapicorn-site.tar.bz2
	rm -rf ${RAPICORN_PREVIEW}/html/
	tar jxmpf rapicorn-site.tar.bz2 -C ${RAPICORN_PREVIEW}/
	: #ln -s main.html ${RAPICORN_PREVIEW}/html/index.html
.PHONY: site-preview

# remote installation
REMOTE=rapicorn.org.^.private
REMOTE_DEST=/srv/rapicorn.org/
REMOTE_TMP=_scptmp
site-update: rapicorn-site.tar.bz2
	scp -p $< ${REMOTE}:${REMOTE_TMP}-$(<F)
	ssh ${REMOTE}						\
	 "set -ex "						\
	 "&& tar jxmpf ${REMOTE_TMP}-$(<F) -C ${REMOTE_DEST} "	\
	 "&& rm ${REMOTE_TMP}-$(<F)"
.PHONY: site-update
