# Rapicorn                                       -*- Mode: shell-script; -*-

builtin(include, acrapicorn.m4)dnl # include special macros
AC_PREREQ(2.57)

# == Package & Version ==
# VERSION: MAJOR.MINOR.MICRO-RC:
# * MAJOR:           Increment on major API breaks, *forces* ABI break, resets MINOR, MICRO and ABI_VERSION
# * MINOR:           Increment for significant changes or ABI breaks, resets MICRO
# * MICRO:           Increment with every release, increments must *not* break ABI
# * ABI_VERSION:     Record the latest version since ABI compatibility has been kept
# * ABI changes:     Increment MAJOR or MINOR and reset ABI_VERSION to the current VERSION
# * ~RC:             Release candidate, may contain [~-]\w+[0-9]+ or nothing
AC_INIT([rapicorn], [17.0.0~rc1])
:;      ABI_VERSION=[17.0.0]
AC_CONFIG_SRCDIR([ui/widget.hh])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_CONFIG_HEADER(config/config.h)
AC_CANONICAL_TARGET                                     # architecture information
AM_INIT_AUTOMAKE(1.9 no-dist no-define foreign)
# version handling
RAPICORN_VERSION="$PACKAGE_VERSION"
[MAJOR=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\1/'`]
[MINOR=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\2/'`]
[MICRO=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\3/'`]
AC_SUBST(RAPICORN_VERSION)
AC_DEFINE_UNQUOTED(RAPICORN_VERSION, "$RAPICORN_VERSION", [Rapicorn Version])
# For automake, we need to easily provide MAJOR.MINOR.MICRO and MAJOR_MINOR_MICRO:
AC_SUBST(MAJOR)
AC_SUBST(MINOR)
AC_SUBST(MICRO)
# calculate ABI age
ABI_MAJOR=[`echo "$ABI_VERSION" | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\1/'`]
ABI_MINOR=[`echo "$ABI_VERSION" | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\2/'`]
ABI_MICRO=[`echo "$ABI_VERSION" | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\3/'`]
test "$ABI_MAJOR" = "$MAJOR" || {
  AC_MSG_ERROR([ABI_VERSION must match MAJOR version: $ABI_MAJOR == $MAJOR])
}
test "$ABI_MINOR" -le "$MINOR" || {
  AC_MSG_ERROR([ABI_VERSION must predate MINOR version: $ABI_MINOR <= $MINOR])
}
test "$ABI_MICRO" -eq 0 || {
  AC_MSG_ERROR([ABI_VERSION must not indicate ABI break in MICRO releases: $ABI_MAJOR.$ABI_MINOR.$ABI_MICRO == $ABI_MAJOR.$ABI_MINOR])
}
ABI_AGE=$(($MINOR - $ABI_MINOR))
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# Note, this package provides backwards binary compatibility only within
# MINOR.MICRO increments, and only since ABI_VERSION.
# Interface additions occour with pretty much every release.
# For libtool this means:
# - the library name needs to encode MAJOR
# - the '-release X.Y' option cannot be used, because it's ignored for naming .a, .la, .so files.
# - lt_current=MINOR  # disallow forward compatible builds, i.e. enforce runtime-lib >= build-time-lib
# - lt_age=ABI_AGE    # allow backward compatibility between ABI_VERSION .. VERSION
# - lt_revision=MICRO # pick the latest SO revision for unchanged MAJOR.MINOR,
#                     # see also: https://autotools.io/libtool/version.html
# On Linux this yields:
# library:      libfoo-$MAJOR.so.$ABI_MINOR.$ABI_AGE.$MICRO
# SONAME:       libfoo-$MAJOR.so.$ABI_MINOR
lt_current=$MINOR ; lt_age=$ABI_AGE ; lt_revision=$MICRO
LT_VERSION_INFO="$lt_current:$lt_revision:$lt_age"
AC_SUBST(LT_VERSION_INFO)

# == Argument parsing ==
AC_ARG_ENABLE(debug, [  --enable-debug=[no/min/yes/asan/tsan/lsan/ubsan] turn on debugging, default=minimum],,enable_debug=min)
# package library defaults
enable_static=no ; enable_shared=yes

# == Check srcdir == builddir ==
echo "checking for srcdir==builddir... (#buildapi-variable-no-builddir)" # see https://github.com/cgwalters/build-api
TEST_SRCDIR=`cd $srcdir ; /bin/pwd -P`
TEST_BUILDDIR=`cd . ; /bin/pwd -P`
test "$TEST_SRCDIR" == "$TEST_BUILDDIR" || {
  AC_MSG_ERROR([builds require builddir == srcdir (use e.g. git-worktree instead)])
}

# == Check for git ==
AC_MSG_CHECKING([for git repository])
DOTGIT=`git rev-parse --git-dir 2>/dev/null` && INGIT= || INGIT='#'
AC_SUBST(INGIT)
result=$(test -z "$INGIT" && echo yes || echo no)
AC_MSG_RESULT($result)

# == Compiler, linker, libtool ==
saved_CFLAGS="$CFLAGS" ; saved_CXXFLAGS="$CFLAGS" # the AC_PROG_C* macros mess up CFLAGS and CXXFLAGS
AC_PROG_CC		# note, this must preceede AC_CHECK_LIB, AC_TRY_RUN, etc to preserve CFLAGS etc
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_LANG_CPLUSPLUS	# use CXX for compilation tests
AC_PROG_LD
AC_LIB_PROG_LD
AC_LIB_PROG_LD_GNU
CFLAGS="$saved_CFLAGS" ; CXXFLAGS="$saved_CXXFLAGS"
AM_PROG_LIBTOOL
AC_LIBTOOL_DLOPEN

# == C++ Dialect ==
CXX="$CXX -std=gnu++11"
test -z "$CXXCPP" || CXXCPP="$CXXCPP -std=gnu++11"

# == Compiler flags ==
# needs gcc >= 4.8, clang >= 3.4
c_warnings="-Wall -Wdeprecated -Werror=format-security -Wredundant-decls -Wpointer-arith -Wmissing-declarations" # -Wdate-time -Wconversion -Wshadow
cxx_warnings="-Wsign-promo -Woverloaded-virtual" # -Wnon-virtual-dtor  -Wempty-body -Wignored-qualifiers -Wunreachable-code -Wtype-limits
c_only_flags="-Wmissing-prototypes -Wnested-externs -Wno-pointer-sign"
gcc_c_only_flags="-fno-cond-mismatch -fdevirtualize-speculatively"
compiler_flags="-pipe"
optimization_flags="-O3 -funroll-loops -ftree-vectorize"
FAST_MATH_FLAGS="-ffast-math"
gcc_flags="-rdynamic -finline-functions -ftracer -ftree-loop-distribution -ftree-loop-ivcanon -ftree-loop-im"
gcc_fast_math="-fivopts"
clang_fix_flags="-Qunused-arguments"		# avoid bogus -pthread warning (llvm#7798)
x86_flags="-minline-all-stringops"
x86_flags="$x86_flags -mmmx -msse -msse2"	# Intel since 2001, AMD since 2003
x86_flags="$x86_flags -msse3"			# Intel since 2004, AMD since 2007
#x86_flags="$x86_flags -mssse3"			# Intel since 2006, AMD since 2011
x86_64_flags="-mcx16"				# for CMPXCHG16B, in AMD64 since 2005
#x86_64_flags="$x86_64_flags -msse4a"		# AMD since 2007
#x86_64_flags="$x86_64_flags -msse4.1 -msse4.2"	# Intel since 2008, AMD since 2011
#x86_64_flags="$x86_64_flags -mavx"		# Intel since 2011, AMD since 2011
#x86_64_flags="$x86_64_flags -mavx2"		# Intel since 2013, AMD since 2015
if   ${CXX} --version 2>&1 | grep -q -F 'clang' ; then
    AM_CFLAGS="$c_warnings $c_only_flags $compiler_flags $optimization_flags $clang_fix_flags"
    AM_CXXFLAGS="$c_warnings $cxx_warnings $compiler_flags $optimization_flags $clang_fix_flags"
    MC_ASSERT_VERSION($CXX --version, 3.5.0)
elif ${CXX} --version 2>&1 | tr '\n' ' ' | grep -q -E 'Free Software Foundation' ; then
    AM_CFLAGS="$c_warnings $c_only_flags $gcc_c_only_flags $compiler_flags $optimization_flags $gcc_flags"
    AM_CXXFLAGS="$c_warnings $cxx_warnings $compiler_flags $optimization_flags $gcc_flags"
    FAST_MATH_FLAGS="$FAST_MATH_FLAGS $gcc_fast_math"
    MC_ASSERT_VERSION($CXX --version, 4.9.2)
else
  AC_MSG_ERROR([unknown C++ compiler])
fi
AC_SUBST(FAST_MATH_FLAGS)
uname_m=`uname -m`
if test "$uname_m" = x86_64 ; then	# AMD64
    AM_CFLAGS="$AM_CFLAGS $x86_flags $x86_64_flags"
    AM_CXXFLAGS="$AM_CXXFLAGS $x86_flags $x86_64_flags"
elif echo "$uname_m" | grep -q '^i.*86$' ; then
    AM_CFLAGS="$AM_CFLAGS $x86_flags"
    AM_CXXFLAGS="$AM_CXXFLAGS $x86_flags"
fi

# == Debug flags ==
case "_$enable_debug" in
  _ubsan)
    debug_flags="-DG_ENABLE_DEBUG -ggdb3 -fverbose-asm -fno-omit-frame-pointer -fno-inline -fsanitize=undefined"
    AM_LDFLAGS="$AM_LDFLAGS -lubsan" ;;
  _asan)
    debug_flags="-DG_ENABLE_DEBUG -ggdb3 -fverbose-asm -fno-omit-frame-pointer -fno-inline -fsanitize=address"
    AM_LDFLAGS="$AM_LDFLAGS -lasan" ;;
  _tsan)
    debug_flags="-DG_ENABLE_DEBUG -ggdb3 -fverbose-asm -fno-omit-frame-pointer -fno-inline -fsanitize=thread"
    AM_LDFLAGS="$AM_LDFLAGS -ltsan" ;;
  _lsan)
    debug_flags="-DG_ENABLE_DEBUG -ggdb3 -fverbose-asm -fno-omit-frame-pointer -fno-inline -fsanitize=leak"
    AM_LDFLAGS="$AM_LDFLAGS -llsan" ;;
  _yes)
    debug_flags="-DG_ENABLE_DEBUG -ggdb3 -fverbose-asm -fno-omit-frame-pointer" ;;
  _min*)
    debug_flags="-DG_ENABLE_DEBUG -g1" ;;
  _*)
    debug_flags="-DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS" ;; # -DG_DISABLE_ASSERT
esac
AM_CFLAGS="$AM_CFLAGS $debug_flags"
AM_CXXFLAGS="$AM_CXXFLAGS $debug_flags"
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(AM_CPPFLAGS)

# == Linker flags ==
AM_LDFLAGS="$AM_LDFLAGS -lrt -pthread"
ld_help_text=`${LD-ld} --help 2>&1 | tr '\n' ' '`
check_ld_flags="-export-dynamic -O1 -Bsymbolic-functions --as-needed --no-undefined"
for flag in $check_ld_flags ; do
  echo " $ld_help_text" | grep -q -e "$flag" && AM_LDFLAGS="$AM_LDFLAGS -Wl,$flag"
done
echo " $ld_help_text" | grep -q -e "--hash-style" && AM_LDFLAGS="$AM_LDFLAGS -Wl,--hash-style=both"
AC_SUBST(AM_LDFLAGS)

# == ENABLE_DEVEL_MODE ==
# check for development build rules, which require acess to .git/
enable_devel_mode=0
if echo " $PACKAGE_VERSION" | grep -q '~wip' ; then
    enable_devel_mode=1
fi
AC_DEFINE_UNQUOTED(ENABLE_DEVEL_MODE, $enable_devel_mode, [Add development aids to the build])
AM_CONDITIONAL(ENABLE_DEVEL_MODE, test x"$enable_devel_mode" = x1)

# == Utilities and build tools ==
AC_PROG_MAKE_SET()
AM_SILENT_RULES([yes])
AC_PROG_LN_S
AC_PROG_INSTALL
MC_ASSERT_PROG(PKG_CONFIG, pkg-config)
MC_ASSERT_PROG(FLEX, flex)
MC_ASSERT_PROG(BISON, bison)
MC_ASSERT_PROG(PANDOC, pandoc)
MC_ASSERT_PROG(DOXYGEN, doxygen)
MC_ASSERT_PROG(DOT, dot)
MC_ASSERT_PROG(PYTHON2, python-2.7 python2.7 python2)
MC_ASSERT_VERSION($PYTHON --version, 2.7)
MC_ASSERT_PROG(CYTHON, cython)
MC_ASSERT_VERSION($CYTHON --version, 0.20.1)
MC_ASSERT_PROG(XMLLINT, xmllint)
MC_ASSERT_VERSION($XMLLINT --version, 20901)

# == Headers ==
AC_CHECK_HEADERS_ONCE( zlib.h sys/eventfd.h readline/readline.h readline/history.h )

# == pkg-config Libraries ==
# librapicorn dependencies
LIBRAPICORN_REQUIRES="
	libpng		>= 1.2.50
	cairo		>= 1.10.0
	librsvg-2.0	>= 2.40.2
	pango		>= 1.30.0
	pangoft2	>= 1.30.0
	pangocairo	>= 1.30.0
	x11		>= 1.6.2
	xext		>= 1.3.3
	glib-2.0	>= 2.32.0
	gio-2.0		>= 2.30.0
	gthread-2.0	>= 2.30.0
"
LIBRAPICORN_REQUIRES=`echo $LIBRAPICORN_REQUIRES` # strip whitespaces
AC_SUBST(LIBRAPICORN_REQUIRES)
# define LIBRAPICORN_CFLAGS and LIBRAPICORN_LIBS
PKG_CHECK_MODULES(LIBRAPICORN, [ $LIBRAPICORN_REQUIRES ])
# for C++ library linking, '-lpthread' is needed in addition to '-pthread'
LIBRAPICORN_LIBS=`printf %s "$LIBRAPICORN_LIBS" | sed 's/\(-pthread\b\)/\1 -lpthread/' `

# == libz ==
AC_CHECK_LIB(z, gzsetparams, [ LIBZ_LIBS='' ; test "$ac_cv_header_zlib_h" = yes && LIBZ_LIBS='-lz' ])
if test -z "$LIBZ_LIBS"; then
    AC_MSG_ERROR([Compression library libz is missing, but required])
else
  LIBRAPICORN_LIBS="$LIBRAPICORN_LIBS $LIBZ_LIBS"
fi
AC_SUBST(LIBZ_LIBS)

# == libreadline & libhistory ==
READLINELIBS=""
AC_CHECK_LIB(readline, readline,
	     [
	       AC_CHECK_LIB(history, using_history,
			    [
			      test "$ac_cv_header_readline_readline_h$ac_cv_header_readline_history_h" = yesyes && {
				AC_DEFINE_UNQUOTED(HAVE_READLINE_AND_HISTORY, 1, [Whether -lreadline -lhistory works])
				READLINELIBS="-lreadline -lhistory"
			      }
			    ])
	     ])
AC_SUBST(READLINELIBS)
test -z "$READLINELIBS" && {
  AC_MSG_ERROR(Missing readline library development files)
}

# == sizeof ==
AC_CHECK_SIZEOF(uint, 0, [#include <sys/types.h>])
RAPICORN_SIZEOF_SYS_TYPESH_UINT="$ac_cv_sizeof_uint"
AC_SUBST(RAPICORN_SIZEOF_SYS_TYPESH_UINT) # needed by sys-config.h.in

# == RAPICORN_SYSVAL_POLLINIT ==
AC_MSG_CHECKING([for POLL constants])
rm -f conftest.val
rapicorn_pollvalue_code='
  #define _GNU_SOURCE
  #include <sys/types.h>
  #include <sys/poll.h>
  #include <stdio.h>
int main() {
  FILE *f = fopen ("conftest.val", "w");
  if (!f)
    return 1;
  fprintf (f, "0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x",
           POLLIN, POLLPRI, POLLOUT, POLLRDNORM, POLLRDBAND, POLLWRNORM, POLLWRBAND, POLLERR, POLLHUP, POLLNVAL);
  return ferror (f) || fclose (f) != 0;
}'
AC_TRY_RUN([$rapicorn_pollvalue_code], rapicorn_pollvalue_result=`cat conftest.val`, unset rapicorn_pollvalue_result)
rm -f conftest.val
if test -z "$rapicorn_pollvalue_result" ; then
  AC_MSG_RESULT([failed])
  AC_MSG_ERROR([failed to detect poll values])
else
  AC_MSG_RESULT([done])
fi
RAPICORN_SYSVAL_POLLINIT="[ $rapicorn_pollvalue_result ]"
AC_SUBST(RAPICORN_SYSVAL_POLLINIT) # macro needed for a public header file
dnl # AC_DEFINE_UNQUOTED(RAPICORN_SYSVAL_POLLINIT, [((const unsigned int[[]]) { $rapicorn_pollvalue_result })], [Values for POLLIN..POLLNVAL])

# == pthread_spinlock_t initializer ==
AC_LANG_PUSH([C])
saveed_LDFLAGS="$LDFLAGS" ; LDFLAGS="-pthread $LDFLAGS"
AC_TRY_RUN(
  [
    #include <stdio.h>
    #include <string.h>
    #include <assert.h>
    #include <pthread.h>
    struct Spin { pthread_spinlock_t dummy1, s1, dummy2, s2, dummy3; };
    int
    main (int argc, char *argv[])
    {
      struct Spin spin;
      memset (&spin, 0xffffffff, sizeof (spin));
      if (pthread_spin_init (&spin.s1, 0) == 0 &&
          pthread_spin_init (&spin.s2, 0) == 0 &&
          sizeof (pthread_spinlock_t) == 4 && // current implementation assumption
          spin.s1 == spin.s2) // check for location-independent initialization
        {
          FILE *f = fopen ("conftest_spinlock_initializer", "w");
          assert (f);
          fprintf (f, "{ ");
          fprintf (f, "0x%04x,", *(int*) &spin.s1);
          fprintf (f, " }\n");
          fclose (f);
        }
      return 0;
    } ],
  RAPICORN_SPINLOCK_INITIALIZER=`cat conftest_spinlock_initializer` && rm -f conftest_spinlock_initializer ,
  RAPICORN_SPINLOCK_INITIALIZER='' )
LDFLAGS="$saveed_LDFLAGS"
AC_LANG_POP([C])
test -z "$RAPICORN_SPINLOCK_INITIALIZER" && {
  AC_MSG_FAILURE([spinlock initializer check failed to execute])
}
AC_SUBST(RAPICORN_SPINLOCK_INITIALIZER)

# == bin/python & libpython.so & Python.h ==
PYTHON_REQUIRED=2.7
AM_PATH_PYTHON($PYTHON_REQUIRED)
dnl # AC_PYTHON_DEVEL is provided by autoconf-archive
AC_PYTHON_DEVEL([>= $PYTHON_REQUIRED])
AC_MSG_CHECKING([for working Python extension compilation])
PYTHON_CPPFLAGS="$PYTHON_CPPFLAGS -fno-strict-aliasing" # needed by Python headers
saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="-Wall -Werror $CPPFLAGS $PYTHON_CPPFLAGS"     # verify sane Python module builds
AC_TRY_COMPILE([#include <Python.h>], [Py_InitModule(0,0);], py_build=true, py_build=false)
CPPFLAGS="$saved_CPPFLAGS"
AC_MSG_RESULT($py_build)
if test "$py_build" != "true" ; then
    AC_MSG_ERROR([failed to build extension against Python (>=$PYTHON_REQUIRED) headers])
fi

# == I18n ==
### TRANSLATORS: enter your language here
ALL_LINGUAS="de"
### TRANSLATORS: enter your language here
AC_SUBST(ALL_LINGUAS)
# versioned Rapicorn gettext domain (po/)
RAPICORN_GETTEXT_DOMAIN="librapicorn-$MAJOR.$MINOR.$MICRO"
AC_SUBST(RAPICORN_GETTEXT_DOMAIN)
AC_DEFINE_UNQUOTED(RAPICORN_GETTEXT_DOMAIN, "$RAPICORN_GETTEXT_DOMAIN", [Versioned Rapicorn gettext domain])
GETTEXT_PACKAGE=$RAPICORN_GETTEXT_DOMAIN
AC_SUBST(GETTEXT_PACKAGE)
# determine pkglocaledir
saved_prefix="$prefix" ; saved_exec_prefix="$exec_prefix"
test "x$prefix" = xNONE && prefix="$ac_default_prefix"
if test "x$CATOBJEXT" = "x.mo" ; then
    test "x$exec_prefix" = xNONE && exec_prefix="$prefix" # needs $prefix
    pkglocaledir=`eval echo "${libdir}/locale"`           # needs $exec_prefix
else
  datarootdir=`eval echo "${datarootdir}"`		  # needs $prefix
  pkglocaledir=`eval echo "${datadir}/locale"`	          # needs ${datarootdir}
fi
exec_prefix="$saved_exec_prefix" ; prefix="$saved_prefix"
AC_SUBST(pkglocaledir)
# check for gettext, provide INTLLIBS and CATOBJEXT
AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL


# == Output files ==
AC_CONFIG_FILES([
Makefile
po-helper.sh
po/Makefile.in
rcore/sys-config.h
])
AC_OUTPUT

# == Configuration summary ==
printexpandvar()
{
	_x="$1";
	while echo " $_x" | fgrep -q '${'; do
		_x=$(eval echo "$_x")
	done
	echo -n "$_x"
}
cat <<EOF
Autoconfiguration complete.

	Building package:		$PACKAGE-$VERSION
	C++ Namespace:			Rapicorn
	Operating System:		$RAPICORN_OS

	CPU optimizations:		${FPU_FLAGS:+fpu}${MMX_FLAGS:+ mmx}${AMD3DNOW_FLAGS:+ 3dnow}${SSE_FLAGS:+ sse}${SSE2_FLAGS:+ sse2}${SSE3_FLAGS:+ sse3}
	Development mode:		${ENABLE_DEVEL_MODE_FALSE:+enabled}${ENABLE_DEVEL_MODE_TRUE:+disabled}

	Python extension:		enabled (mandatory)
	Readline libs:			$READLINELIBS

	Binaries:			`printexpandvar ${bindir}`
	Libraries:			`printexpandvar ${libdir}`
	Locales:			`printexpandvar ${pkglocaledir}`
	Includes:			`printexpandvar ${includedir}/rapicorn-$MAJOR`
	Data:				`printexpandvar ${datadir}`
	Documents:			`printexpandvar ${docdir}`
	PyRapicorn:			`printexpandvar ${pythondir}/Rapicorn_${MAJOR}`
	Python module:			import Rapicorn_${MAJOR} as Rapicorn

EOF
