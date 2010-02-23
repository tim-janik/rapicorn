# Rapicorn
# Copyright (C) 2007 Tim Janik
## GNU Lesser General Public License version 2 or any later version.
include $(top_srcdir)/Makefile.decl

HTML_TARGETS = $(strip                  	\
	rapicorn-html/				\
	rapicorn-html/rapicorn16.png		\
	rapicorn-html/rapicorn032.png		\
	rapicorn-html/hgradientG.png		\
	rapicorn-html/vgradientG.png		\
	rapicorn-html/style.css			\
	rapicorn-html/doxer-style.css		\
)
RDOCU_TARGETS = $(strip                 	\
	rapicorn-html/rdocu-ui/index.html	\
)

TARGET_DIRS   = rapicorn-html/ tmpdoxi-ui/ rapicorn-html/rdocu-ui/
DOXIDEFS      = -D RAPICORN_VERSION ${RAPICORN_VERSION} -I $(srcdir)
DOCFRAME_DEFS = -D DOXI_INCLUDE_FILE \"definitions.doxi\"    # $(if $(findstring $(<F), ${WEBMENU_DOXI_FILES}), \"webmenu.doxi\", \"webframe.doxi\")

# Reference Documentation
../docs/psource/rapicorn-core.psrc ../docs/psource/rapicorn-gui.psrc: PHONY-TARGET
	@$(MAKE) -C ../docs $(patsubst ../docs/%, %, $@)
# Rapicorn reference docu
tmpdoxi-ui/index.doxi: ../docs/psource/rapicorn-core.psrc ../docs/psource/rapicorn-gui.psrc rdocu-template.doxi definitions.doxi
	rm -f tmpdoxi-ui/*
	$(DOXER) pickle2doxi $(strip -i -D TOP_WEBDIR ".."             		\
		../docs/psource/rapicorn-core.psrc				\
		../docs/psource/rapicorn-gui.psrc				\
		 -d tmpdoxi-ui/ $(DOXIDEFS) -T rdocu-template.doxi 	\
		 -D DOCU_TEMPLATE_TITLE "Rapicorn - Interface Reference"        \
		 -D DOCU_TEMPLATE_HEADING "Rapicorn Interface Reference" )
rapicorn-html/rdocu-ui/index.html: tmpdoxi-ui/index.doxi $(wildcard tmpdoxi-ui/*.doxi)
	rm -f rapicorn-html/rdocu-ui/*
	$(DOXER) doxi2html $(strip                      -D TOP_WEBDIR ".."      \
		 -d rapicorn-html/rdocu-ui/ $(DOXIDEFS) $(call DOCFRAME_DEFS)      \
		 $(filter-out $<, $(shell echo tmpdoxi-ui/*.doxi)) )
	$(DOXER) doxi2html $(strip                      -D TOP_WEBDIR ".."      \
		 -d rapicorn-html/rdocu-ui/ $(DOXIDEFS) $(call DOCFRAME_DEFS) $< )
EXTRA_DIST += rdocu-template.doxi

# rule: mkdir
${TARGET_DIRS}:
	mkdir -p $@
# rule: .doxi -> .html
rapicorn-html/%.html: ./%.doxi definitions.doxi
	$(DOXER) doxi2html $(strip $< -d rapicorn-html/  -D TOP_WEBDIR "." $(DOXIDEFS) $(call DOCFRAME_DEFS))
# rule: .png -> rapicorn-html/
rapicorn-html/%.png: ./%.png
	cp $< $@
# rule: .css -> rapicorn-html/
rapicorn-html/%.css: ./%.css
	cp $< $@
# rule: doxer-style.css
rapicorn-html/doxer-style.css:
	$(DOXER) -d rapicorn-html/ writecss
# rule: *.tar.bz2
rapicorn-site.tar.bz2: ${TARGET_DIRS} ${HTML_TARGETS} ${RDOCU_TARGETS}
	tar jcf $@ --anchored --exclude='rapicorn-html/screenshotdir/*' --exclude='rapicorn-html/logogallerydir/*' rapicorn-html/
CLEANFILES += rapicorn-site.tar.bz2
# rule: site*
site: rapicorn-site.tar.bz2
.PHONY: site
noinst_DATA = @DOXERDEP@ # rapicorn-site.tar.bz2
# cleanup
clean-local:
	rm -f screenshotdir/*.THUMB.jpg
	rm -f ${intermediate_files} ${CLEANFILES}
	rm -f tmpdoxi-ui/* rapicorn-html/rdocu-ui/*
	-rmdir -p tmpdoxi-ui/ rapicorn-html/rdocu-ui/
	rm -fr rapicorn-html/screenshotdir/ rapicorn-html/web-images/ rapicorn-html/images/ rapicorn-html/style/
	rm -f rapicorn-html/* rapicorn-html/.htaccess
	rmdir rapicorn-html/ 2>/dev/null ; true
.PHONY: PHONY-TARGET

# preview installation
RAPICORN_PREVIEW=~/public_rapicorn-html/rapicorn-preview
site-preview: ${RAPICORN_PREVIEW}/. rapicorn-site.tar.bz2
	rm -rf ${RAPICORN_PREVIEW}/rapicorn-html/
	tar jxmpf rapicorn-site.tar.bz2 -C ${RAPICORN_PREVIEW}/
	: #ln -s main.html ${RAPICORN_PREVIEW}/rapicorn-html/index.html
.PHONY: site-preview

# remote installation
REMOTE=rapicorn.org.^.private
REMOTE_DEST=/srv/testbit.eu/
REMOTE_TMP=_scptmp
site-update: rapicorn-site.tar.bz2
	scp -p $< ${REMOTE}:${REMOTE_TMP}-$(<F)
	ssh ${REMOTE}						\
	 "set -ex "						\
	 "&& tar jxmpf ${REMOTE_TMP}-$(<F) -C ${REMOTE_DEST} "	\
	 "&& rm ${REMOTE_TMP}-$(<F)"
.PHONY: site-update
