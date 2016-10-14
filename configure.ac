dnl # Rapicorn                                       -*- Mode: shell-script; -*-
dnl
builtin(include, acrapicorn.m4)dnl # include special macros
builtin(include, ld-symbolic.m4)dnl # include special macros

AC_PREREQ(2.57)
# VERSION: MAJOR.MINOR.MICRO-RC:
# * MAJOR:           Increment on major API breaks, *forces* ABI break, resets MINOR, MICRO and ABI_VERSION
# * MINOR:           Increment for significant changes or ABI breaks, resets MICRO
# * MICRO:           Increment with every release, increments must *not* break ABI
# * ABI_VERSION:     Record the latest version since ABI compatibility has been kept
# * ABI changes:     Increment MAJOR or MINOR and reset ABI_VERSION to the current VERSION
# * ~RC:             Release candidate, may contain [~-]\w+[0-9]+ or nothing
AC_INIT([rapicorn], [16.0.1~wip])
:;      ABI_VERSION=[16.0.0]
AC_CONFIG_SRCDIR([ui/widget.hh])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_CONFIG_HEADER(configure.h)
AC_CANONICAL_TARGET                                     # architecture information
AM_INIT_AUTOMAKE(1.9 dist-xz tar-pax no-dist-gzip no-define foreign)
# version handling
RAPICORN_VERSION="$PACKAGE_VERSION"
[MAJOR=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\1/'`]
[MINOR=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\2/'`]
[MICRO=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\3/'`]
AC_SUBST(RAPICORN_VERSION)
AC_DEFINE_UNQUOTED(RAPICORN_VERSION, "$RAPICORN_VERSION", [Rapicorn Version])
AC_DEFINE_UNQUOTED(RAPICORN_ARCH_NAME, "$target", [Architecture Description])
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
# Note, rapicorn provides backwards binary compatibility only within
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
# library:	librapicorn-$MAJOR.so.$ABI_MINOR.$ABI_AGE.$MICRO
# SONAME:	librapicorn-$MAJOR.so.$ABI_MINOR
lt_current=$MINOR ; lt_age=$ABI_AGE ; lt_revision=$MICRO
LT_VERSION_INFO="$lt_current:$lt_revision:$lt_age"
AC_SUBST(LT_VERSION_INFO)

# == argument parsing ==
AC_ARG_ENABLE(debug,      [  --enable-debug=[no/minimum/yes]   turn on debugging, default=yes],,enable_debug=yes)
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

# == Libraries ==
# libz
AC_CHECK_LIB(z, gzsetparams, [AC_CHECK_HEADER(zlib.h, LIBZ_LIBS='-lz', LIBZ='')], LIBZ='')
if test -z "$LIBZ_LIBS"; then
    AC_MSG_ERROR([Compression library libz is missing, but required])
fi
AC_SUBST(LIBZ_LIBS)
# librapicorn dependencies
LIBRAPICORN_REQUIRES="
	libpng12	>= 1.2.50
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
LIBRAPICORN_LIBS="$LIBRAPICORN_LIBS $LIBZ_LIBS"
# for C++ library linking, '-lpthread' is needed in addition to '-pthread'
LIBRAPICORN_LIBS=`printf %s "$LIBRAPICORN_LIBS" | sed 's/\(-pthread\b\)/\1 -lpthread/' `

# == detect compiler, linker & libtool ==
saved_CFLAGS="$CFLAGS"
AC_PROG_CC
CFLAGS="$saved_CFLAGS" 		# AC_PROG_CC_G called internally consistently breaks $CFLAGS
saved_CXXFLAGS="$CFLAGS"
AC_PROG_CPP
CXXFLAGS="$saved_CXXFLAGS"	# AC_PROG_CXX_G called internally consistently breaks $CXXFLAGS
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_LD
AC_LANG_CPLUSPLUS	# use CXX for compilation tests
AC_LIB_PROG_LD
AC_LIB_PROG_LD_GNU
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
gcc_flags="-rdynamic -finline-functions -ftracer -ftree-loop-distribution"
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
    AM_CFLAGS="$c_warnings $c_only_flags $compiler_flags $optimization_flags"
    AM_CXXFLAGS="$c_warnings $cxx_warnings $compiler_flags $optimization_flags"
    MC_ASSERT_VERSION($CXX --version, 3.5.0)
elif ${CXX} --version 2>&1 | tr '\n' ' ' | grep -q -E '(g\++|GCC).*Free Software Foundation' ; then
    AM_CFLAGS="$c_warnings $c_only_flags $gcc_c_only_flags $compiler_flags $optimization_flags $gcc_flags"
    AM_CXXFLAGS="$c_warnings $cxx_warnings $compiler_flags $optimization_flags $gcc_flags"
    MC_ASSERT_VERSION($CXX --version, 4.9.2)
else
  AC_MSG_ERROR([unknown C++ compiler])
fi
uname_m=`uname -m`
if test "$uname_m" = x86_64 ; then	# AMD64
    AM_CFLAGS="$AM_CFLAGS $x86_flags $x86_64_flags"
    AM_CXXFLAGS="$AM_CXXFLAGS $x86_flags $x86_64_flags"
elif echo "$uname_m" | grep -q '^i.*86$' ; then
    AM_CFLAGS="$AM_CFLAGS $x86_flags"
    AM_CXXFLAGS="$AM_CXXFLAGS $x86_flags"
fi
if test "x$enable_debug" = "xyes"; then
    AM_CFLAGS="$AM_CFLAGS -DG_ENABLE_DEBUG -g3 -fno-omit-frame-pointer -fverbose-asm"
    AM_CXXFLAGS="$AM_CXXFLAGS -DG_ENABLE_DEBUG -g3 -fno-omit-frame-pointer -fverbose-asm"
elif test "x$enable_debug" = "xno"; then
    AM_CFLAGS="$AM_CFLAGS -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS" # -DG_DISABLE_ASSERT
    AM_CXXFLAGS="$AM_CXXFLAGS -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS" # -DG_DISABLE_ASSERT
fi
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CXXFLAGS)

# == Linker flags ==
AM_LDFLAGS="-lrt -pthread"
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
RAPICORN_SYSVAL_POLLINIT="[((const unsigned int[]) { $rapicorn_pollvalue_result })]"
AC_SUBST(RAPICORN_SYSVAL_POLLINIT) # macro needed for a public header file
dnl # AC_DEFINE_UNQUOTED(RAPICORN_SYSVAL_POLLINIT, [((const unsigned int[[]]) { $rapicorn_pollvalue_result })], [Values for POLLIN..POLLNVAL])


dnl # RAPICORN_CORE_REQUIREMENTS() - checks everything needed for rcore
AC_DEFUN([RAPICORN_CORE_REQUIREMENTS],
[
  # === docdir ===
  # AM default: docdir = ${datarootdir}/doc/${PACKAGE_TARNAME} # == ${prefix}/share/doc/rapicorn
  # We need a properly versioned path: ${prefix}/share/doc/rapicorn-MAJOR
  docdir="${datarootdir}/doc/rapicorn" # "rapicorn-$MAJOR"
  AC_SUBST(docdir)

    # --- sizeof and presence of uint ---
    GLIB_SIZEOF([#include <sys/types.h>], uint, rapicorn_sys_typesh_uint)
    RAPICORN_SIZEOF_SYS_TYPESH_UINT="$glib_cv_sizeof_rapicorn_sys_typesh_uint"
    AC_SUBST(RAPICORN_SIZEOF_SYS_TYPESH_UINT)
    GLIB_SIZEOF([], void**, rapicorn_pointer)
    RAPICORN_SIZEOF_POINTER="$glib_cv_sizeof_rapicorn_pointer"
    AC_SUBST(RAPICORN_SIZEOF_POINTER)
    GLIB_SIZEOF([], long, rapicorn_long)
    RAPICORN_SIZEOF_LONG="$glib_cv_sizeof_rapicorn_long"
    AC_SUBST(RAPICORN_SIZEOF_LONG)

    AC_CHECK_HEADERS( [sys/eventfd.h] )

    # --- OS/Win32 detection ---
    dnl # needs AC_CANONICAL_HOST
    test -z "$host" && {
	missing_macro="AC""_CANONICAL_HOST"
	AC_MSG_ERROR([configure failed to execute $missing_macro])
    }
    AC_MSG_CHECKING([for Win32])
    RAPICORN_OS_CHOICES="WIN32 UNIX" # OS types we will check for
    case "$host" in
	*-*-mingw*)
	    RAPICORN_OS=WIN32
	    AC_MSG_RESULT([yes])
	    ;;
	*)
	    RAPICORN_OS=UNIX
	    AC_MSG_RESULT([no])
	    ;;
    esac
    AC_SUBST(RAPICORN_OS)
    AC_SUBST(RAPICORN_OS_CHOICES)
    AC_DEFINE_UNQUOTED(RAPICORN_OS_$RAPICORN_OS, "1", [Win32 detection])

    dnl # == sizeof threading structs ==
    GLIB_SIZEOF([#include <pthread.h>], pthread_mutex_t, pth_mutex_t)
    RAPICORN_SIZEOF_PTH_MUTEX_T="$glib_cv_sizeof_pth_mutex_t"
    AC_SUBST(RAPICORN_SIZEOF_PTH_MUTEX_T)
    GLIB_SIZEOF([#include <pthread.h>], pthread_cond_t, pth_cond_t)
    RAPICORN_SIZEOF_PTH_COND_T="$glib_cv_sizeof_pth_cond_t"
    AC_SUBST(RAPICORN_SIZEOF_PTH_COND_T)

    # == sizeof pthread_rwlock_t ==
    GLIB_SIZEOF([#include <pthread.h>], pthread_rwlock_t, rapicorn_pthreadh_rwlock)
    RAPICORN_SIZEOF_PTHREADH_RWLOCK="$glib_cv_sizeof_rapicorn_pthreadh_rwlock"
    AC_SUBST(RAPICORN_SIZEOF_PTHREADH_RWLOCK)

    # == sizeof pthread_spinlock_t ==
    GLIB_SIZEOF([#include <pthread.h>], pthread_spinlock_t, rapicorn_pthreadh_spinlock)
    RAPICORN_SIZEOF_PTHREADH_SPINLOCK="$glib_cv_sizeof_rapicorn_pthreadh_spinlock"
    AC_SUBST(RAPICORN_SIZEOF_PTHREADH_SPINLOCK)

    # == pthread_spinlock_t initializer ==
    MC_DEFINE_SPINLOCK_INITIALIZER()
    RAPICORN_SPINLOCK_INITIALIZER="$mc_cv_spinlock_initializer"
    AC_SUBST(RAPICORN_SPINLOCK_INITIALIZER)

    dnl # --- pthread_mutexattr_settype ---
    AC_MSG_CHECKING([for pthread_mutexattr_settype()])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      #define _XOPEN_SOURCE   600
      #include <pthread.h>
    ]], [[
      int (*attr_settype) (pthread_mutexattr_t *__attr, int __kind) = pthread_mutexattr_settype;
      int val = PTHREAD_MUTEX_RECURSIVE;
      attr_settype = 0; val = 0;
    ]])],[
      RAPICORN_HAVE_MUTEXATTR_SETTYPE=1
      AC_MSG_RESULT(yes)
    ],[
      RAPICORN_HAVE_MUTEXATTR_SETTYPE=0
      AC_MSG_RESULT(no)
    ])
    AC_SUBST(RAPICORN_HAVE_MUTEXATTR_SETTYPE)

    dnl # == g++<=4.7 missing --enable-libstdcxx-time ==
    AC_CHECK_FUNCS([nanosleep sched_yield])
    if test "x$ac_cv_func_nanosleep$ac_cv_func_sched_yield" = xyesyes; then
      AC_MSG_CHECKING([whether g++ <= 4.7 sleep_for/yield workaround is needed])
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
	      	#if __GNUC__ == 4 && __GNUC_MINOR__ <= 7 && !defined (_GLIBCXX_USE_SCHED_YIELD) && !defined (_GLIBCXX_USE_NANOSLEEP)
	        #  define _GLIBCXX_USE_SCHED_YIELD 1
	        #  define _GLIBCXX_USE_NANOSLEEP 1
	        #  define FAKE_USE_NANOSLEEP__FAKE_USE_SCHED_YIELD 1
		#  include <chrono>
		#  include <thread>
	        #endif
		],[
	      	#if __GNUC__ == 4 && __GNUC_MINOR__ <= 7 && FAKE_USE_NANOSLEEP__FAKE_USE_SCHED_YIELD == 1
		  std::this_thread::sleep_for(std::chrono::milliseconds(200));
		  std::this_thread::yield();
		#else
		  SKIP - TEST; not a buggy gcc version detected;
		#endif
		])], [glibcxx_needs_nanosleep=yes], [glibcxx_needs_nanosleep=no])
      if test "x$glibcxx_needs_nanosleep" = xyes ; then
	AC_MSG_RESULT(yes)
	AC_DEFINE([_GLIBCXX_USE_SCHED_YIELD], [1], [Define if g++<=4.7 fails to detect sched_yield.])
	AC_DEFINE([_GLIBCXX_USE_NANOSLEEP],   [1], [Define if g++<=4.7 fails to detect nanosleep.])
      else
	AC_MSG_RESULT(no)
      fi
    fi
])

dnl # RAPICORN_I18N_REQUIREMENTS() - check for all i18n requirements
AC_DEFUN([RAPICORN_I18N_REQUIREMENTS],
[
    dnl # TRANSLATORS: enter your language here
    ALL_LINGUAS="de"
    dnl # TRANSLATORS: enter your language here
    AC_SUBST(ALL_LINGUAS)

    dnl # versioned Rapicorn gettext domain (po/)
    RAPICORN_GETTEXT_DOMAIN="librapicorn-$MAJOR.$MINOR.$MICRO"
    AC_SUBST(RAPICORN_GETTEXT_DOMAIN)
    AC_DEFINE_UNQUOTED(RAPICORN_GETTEXT_DOMAIN, "$RAPICORN_GETTEXT_DOMAIN", [Versioned Rapicorn gettext domain])
    GETTEXT_PACKAGE=$RAPICORN_GETTEXT_DOMAIN
    AC_SUBST(GETTEXT_PACKAGE)

    dnl # locale directory for all domains
    dnl # (AM_GLIB_DEFINE_LOCALEDIR() could do this if it would do AC_SUBST())
    saved_prefix="$prefix" ; saved_exec_prefix="$exec_prefix"
    test "x$prefix" = xNONE && prefix="$ac_default_prefix"
    if test "x$CATOBJEXT" = "x.mo" ; then
      test "x$exec_prefix" = xNONE && exec_prefix="$prefix" # needs $prefix
      pkglocaledir=`eval echo "${libdir}/locale"` # needs $exec_prefix
    else
      datarootdir=`eval echo "${datarootdir}"`		# needs $prefix
      pkglocaledir=`eval echo "${datadir}/locale"`	# needs ${datarootdir}
    fi
    exec_prefix="$saved_exec_prefix" ; prefix="$saved_prefix"
    AC_SUBST(pkglocaledir)

    dnl # do gettext checks, provide INTLLIBS and CATOBJEXT
    AM_GLIB_GNU_GETTEXT
    AC_PROG_INTLTOOL
])

dnl # RAPICORN_PYTHON_REQUIREMENTS() - rapicorn python checks
AC_DEFUN([RAPICORN_PYTHON_REQUIREMENTS],
[
  # python
  PYTHON_REQUIRED=2.7
  AM_PATH_PYTHON($PYTHON_REQUIRED)
  dnl # AC_REQUIRE([AC_PYTHON_DEVEL]) # AC_PYTHON_DEVEL is usually provided by autoconf-archive
  AC_PYTHON_DEVEL([>= $PYTHON_REQUIRED])
  AC_MSG_CHECKING([for working Python extension compilation])
  saved_CPPFLAGS="$CPPFLAGS"
  PYTHON_CPPFLAGS="$PYTHON_CPPFLAGS -fno-strict-aliasing" # needed by Python headers
  CPPFLAGS="-Wall -Werror $CPPFLAGS $PYTHON_CPPFLAGS"     # verify sane Python module builds
  AC_TRY_COMPILE([#include <Python.h>], [Py_InitModule(0,0);], py_build=ok, py_build=failed)
  CPPFLAGS="$saved_CPPFLAGS"
  AC_MSG_RESULT($py_build)
  if test "$py_build" != "ok" ; then
    AC_MSG_ERROR([failed to build extension against Python (>=$PYTHON_REQUIRED) headers])
  fi
])

dnl # RAPICORN_TEST_REQUIREMENTS() - checks for test programs
AC_DEFUN([RAPICORN_TEST_REQUIREMENTS],
[
    READLINELIBS=""
    AC_CHECK_HEADERS(readline/readline.h, [
        AC_CHECK_HEADERS(readline/history.h, [
	    AC_CHECK_LIB(readline, readline, [
		AC_CHECK_LIB(history, using_history, [
		    AC_DEFINE_UNQUOTED(HAVE_READLINE_AND_HISTORY, 1,
			[Whether -lreadline -lhistory works])
		    READLINELIBS="-lreadline -lhistory"
		    ]) ]) ]) ])
    test -z "$READLINELIBS" && {
      AC_MSG_ERROR(Missing readline library development files)
    }
    AC_SUBST(READLINELIBS)
])

# check requirement sets
RAPICORN_I18N_REQUIREMENTS
RAPICORN_CORE_REQUIREMENTS
RAPICORN_PYTHON_REQUIREMENTS
RAPICORN_TEST_REQUIREMENTS

# create output files
AC_CONFIG_FILES([
Makefile
po-helper.sh
po/Makefile.in
rcore/buildconfig.h
])

AC_OUTPUT

printexpandvar()
{
	_x="$1";
	while echo " $_x" | fgrep -q '${'; do
		_x=$(eval echo "$_x")
	done
	echo -n "$_x"
}

dnl # Show configuration summary
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
