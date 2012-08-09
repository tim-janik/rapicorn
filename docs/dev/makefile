# Build and upload development documentation

all: build-stamp
build-stamp:
	$(MAKE) all-docs
	touch $@

top_srcdir  = ../..
git_stamp   = $(wildcard $(top_srcdir)/.git/$(shell git symbolic-ref -q HEAD) $(top_srcdir)/.git/packed-refs)
GBR_VERSION   = $(shell git name-rev HEAD 2>/dev/null | sed 's/.* //')
PRJ_VERSION   = $(shell sed -ne "/^[ \t]*VERSION[ \t]*=/ { s/^[^=]*=[ \t]*//; p; q }" < $(top_srcdir)/Makefile)
DOC_VERSION = $(or $(GBR_VERSION), $(PRJ_VERSION))
SED_MKEVEN  = sed 's/1$$/0/;s/3$$/2/;s/5$$/4/;s/7$$/6/;s/9$$/8/'

DOC_FILES   = rapicorn-core.hh rapicorn.hh
DOC_DIRS    = rcore/ plic/ rope/ tools/ ui/ docs/tutorial/
DOCU_GLOBS  = *.dox *.py $(DOCU_HGLOBS)
DOCU_HGLOBS = *.idl *.h *.H *.hh *.proc
DOCU_CGLOBS = $(DOCU_GLOBS) *.c *.C *.cc
SRCTREE     = srctree
DOXYGENV    = doxygen-1.7.4
DOXYGEN     = $(if $(shell which $(DOXYGENV)), $(DOXYGENV), doxygen)

EXCLUDES = rope/cpy2rope.cc rope/cxx-client.[hc][hc] \
	rcore/rapicornsignalvariants.hh

.PHONY: $(SRCTREE)
$(SRCTREE):
	rm -rf $(SRCTREE)/ && mkdir -p $(SRCTREE)/
	@echo "### Copying Source..." \
	&& CWD=`pwd` \
	&& for dir in $(DOC_DIRS) ; do \
	  (mkdir -p $$CWD/$(SRCTREE)/$$dir/ \
	   && cd $(top_srcdir)/$$dir \
	   && find . -maxdepth 1 -type f \( $(patsubst %,-name '%' -o, $(DOCU_CGLOBS)) -false \) \
	      -exec cp {} $$CWD/$(SRCTREE)/$$dir/{} \; ) || exit $? ; \
	  done \
	$(foreach FILE, $(DOC_FILES), \
	  && cp $(top_srcdir)/$(FILE) $(SRCTREE)/$(dir $(FILE)))
	@echo "### Identifying Documentation files..." \
	&& cd $(SRCTREE)/ \
	&& find . -type f \( $(patsubst %,-name '%' -o, $(DOCU_GLOBS)) -false \) \
	   -exec sed  '1,+0s,^,/** @file {} */,' {} --in-place \;
	@echo "### Skipping generated Enums..." \
	&& cd $(SRCTREE)/ \
	&& sed 's/^enum\b/#ifndef DOXYGEN\nenum/ ; s/\(operator>>=.*\.pop_evalue().*\)/\1\n#endif/' \
	    */clientapi.hh */serverapi.hh --in-place
	@echo "### Pruning excludes..." \
	&& rm -f $(patsubst %, $(SRCTREE)/%, $(EXCLUDES))
	@echo "### Forcing enum value docs..." \
	&& find $(SRCTREE) -type f \( $(patsubst %,-name '%' -o, $(DOCU_HGLOBS)) -false \) -print0 \
	| xargs -0 python enumdoc.py

STRIP_SNIPPET_MARKS = \
	-e '/^ *<span class="comment"> *\/\/\/ *\[[A-Za-z0-9+_-]\+-EXAMPLE\] *<\/span> *$$/d' \
	-e   's,<span class="comment"> */// *\[[A-Za-z0-9+_-]\+-EXAMPLE\] *</span>\n\?,,' \
	-e '/^ *\(<span class="preprocessor"> *\)\?\#  *\[[A-Za-z0-9+_-]\+-EXAMPLE\]\( *<\/span>\)\? *$$/d' \
	-e 's, *<span class="preprocessor"> *\#  *\[[A-Za-z0-9+_-]\+-EXAMPLE\] *<\/span> *\n\?,,'

all-docs: $(git_stamp) doxygen.cfg
	rm -rf html.tmp/ && mkdir -p html.tmp/
	$(MAKE) $(SRCTREE)
	@echo "### Doxygen..."
	(cat doxygen.cfg \
	&& echo "STRIP_FROM_PATH  = `pwd`/$(SRCTREE)" \
	&& echo "INPUT		  = $(SRCTREE)/" \
	&& echo "EXAMPLE_PATH	  = $(top_srcdir)/" \
	&& echo "OUTPUT_DIRECTORY = html.tmp/" \
	&& echo -n "PROJECT_NUMBER = $(PRJ_VERSION)" \
	) | nice $(DOXYGEN) -
	rm -rf $(SRCTREE)/
	sed $(STRIP_SNIPPET_MARKS) -i html.tmp/html/*.html
	mv html.tmp/html/ html.tmp/$(DOC_VERSION)/
	test "$(DOC_VERSION)" != "master" || ln -s $(DOC_VERSION) html.tmp/latest
	rsync -aH --del html.tmp/ html/
	rm -rf html.tmp/

upload: all
	rsync -zaHP --del '--filter=P /*/' html/  testbit:/srv/dev/html/rapicorn/

clean:
	rm -f build-stamp
	rm -rf $(SRCTREE)/ html.tmp/
	rm -rf html/
maintainer-clean: clean
