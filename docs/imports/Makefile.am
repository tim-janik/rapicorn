# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl

# = Check Tools =
CHECK_WGET = { command -v wget >/dev/null && { echo "1.10.0" ; wget --version 2>&1 | sed 's/[^0-9]*// ; 1q' ; } | sort -VC ; }
CHECK_WH2M = { command -v wikihtml2man >/dev/null && { echo "11.09.1" ; wikihtml2man --version 2>&1 | sed 's/\([^0-9]\|l2m\)*// ; 1q' ; } | sort -VC ; }
check-tools:
	$(Q) ${CHECK_WGET} || { echo "$@: failed to detect recent version: wget"; exit 1; }
	$(Q) ${CHECK_WH2M} || { echo "$@: failed to detect recent version: wikihtml2man"; exit 1; }

# = TAGFILES =
TAGFILES = tagfile-susv4.xml.bz2 cppreference-doxygen-web.tag.xml.bz2
cppreference-doxygen-web.tag.xml.bz2:
	$(AM_V_GEN)
	$(Q) : # wget -k needs the final target name
	$(Q) wget -nv -k --no-cache "http://upload.cppreference.com/mwiki/images/f/f8/cppreference-doxygen-web.tag.xml"
	$(Q) bzip2 -9 cppreference-doxygen-web.tag.xml
EXTRA_DIST += $(TAGFILES)
