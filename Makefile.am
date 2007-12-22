# PLIC - Pluggable IDL Compiler
include $(top_srcdir)/Makefile.decl

SUBDIRS = . plic
noinst_DATA =

# === setup ===
AUTOMAKE_OPTIONS = 1.9 dist-bzip2 no-dist-gzip

# === ChangeLog ===
ChangeLog:	$(shell ls "$${GIT_DIR:-.git}/`git-symbolic-ref -q HEAD || echo HEAD`" 2>/dev/null )
	$(strip \
	  git-log --pretty='format:%ad %an 	# %H (%cn)%n%n%s%n%n%b' \
	  | sed 's/^/	/;s/^	//;/^[ 	]*<unknown>$$/d' > xgen-$(@F) \
	  && cp xgen-$(@F) $@ && rm -f xgen-$(@F) \
	)
noinst_DATA += ChangeLog
EXTRA_DIST  += ChangeLog
