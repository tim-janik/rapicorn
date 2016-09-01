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
# * -RC:             Release candidate, may contain -rc[0-9]+ or nothing
AC_INIT([rapicorn], [16.0.1])
:;      ABI_VERSION=[16.0.0]
AC_CONFIG_SRCDIR([ui/widget.hh])
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

# Build tree checks
AC_MSG_CHECKING([for git repository])
test -f "${srcdir}/.git/index" && INGIT= || INGIT='#'
AC_SUBST(INGIT)
result=$(test -z "$INGIT" && echo yes || echo no)
AC_MSG_RESULT($result)

# prepare automake and make
AC_PROG_MAKE_SET        dnl # support subdirectories.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
mk= ; AC_SUBST([mk]) # workaround for Automake abusing MAKE(1) keywords

# declare --enable-* args and collect ac_help strings
AC_ARG_ENABLE(debug,      [  --enable-debug=[no/minimum/yes]   turn on debugging, default=yes],,enable_debug=yes)
AC_ARG_ENABLE(devel-mode, [  --enable-devel-mode=auto/yes/no build a development version, default=auto],,enable_devel_mode=auto)
# package library defaults
enable_static=no ; enable_shared=yes

# configure requirements
MC_ASSERT_PROG(PKG_CONFIG, pkg-config, [pkg-config can be found at http://pkgconfig.freedesktop.org/])

# check for base package requirements early on
PKG_CHECK_MODULES(SVG_DEPENDENCIES, [
	cairo		>= 1.10.0
	librsvg-2.0	>= 2.40.2
])
PKG_CHECK_MODULES(BASE_DEPENDENCIES, [
	cairo		>= 1.10.0
	glib-2.0	>= 2.32.0
	gio-2.0		>= 2.30.0
	gthread-2.0	>= 2.30.0
	pango		>= 1.30.0
	pangoft2	>= 1.30.0
	pangocairo	>= 1.30.0
])

# compiler, linker & libtool
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
ACX_PROG_LD_GNU_SYMBOLIC
MC_EVAR_ADD(AM_LDFLAGS, $SYMBOLIC_LDFLAGS)
MC_EVAR_ADD(AM_LDFLAGS, -pthread)
MC_EVAR_ADD(AM_LDFLAGS, -lrt)
MC_EVAR_ADD(AM_LDFLAGS, [-Wl,--no-undefined])
if ${LD} --help 2>&1 | grep -q -- '-export-dynamic\b' ; then
  MC_EVAR_ADD(AM_LDFLAGS, [-Wl,-export-dynamic])
fi

# AM_LDFLAGS, optimize linker to use faster ELF symbol hashing if we have compiler and linker support
AC_MSG_CHECKING([whether CC, CXX and LD support gnu.hash sections])
if ${CC}  -dumpspecs 2>&1 | grep -q -- '--hash-style=\(both\|gnu\).*--as-needed' && \
   ${CXX} -dumpspecs 2>&1 | grep -q -- '--hash-style=\(both\|gnu\).*--as-needed' && \
   ${LD}  --help     2>&1 | grep -q -- '--hash-style=.*\(both\|gnu\)' ; then
  MC_EVAR_ADD(AM_LDFLAGS, [-Wl,--hash-style=both -Wl,--as-needed -Wl,-O1])
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST(AM_LDFLAGS)

# Pick C++ dialect, needed in CXXFLAGS for compile tests
MC_CXX_TRY_OPTION(CXXFLAGS,  -std=gnu++11)

# AM_CFLAGS & AM_CXXFLAGS
MC_CXX_TRY_OPTION(AM_CXXFLAGS,  -std=gnu++11)
BOTHCCXXFLAGS=
if test "x$enable_debug" = "xyes"; then
  MC_EVAR_ADD(BOTHCCXXFLAGS,      -DG_ENABLE_DEBUG)
  MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -g3)
  MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -fno-omit-frame-pointer)
  MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -fverbose-asm)
  dnl # MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Q -ftime-report -fmem-report)
elif test "x$enable_debug" = "xno"; then
  MC_EVAR_ADD(BOTHCCXXFLAGS, [-DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS]) # -DG_DISABLE_ASSERT
else
  : # minimum debugging
fi
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Wall)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Wdeprecated)
MC_CC_TRY_OPTION(AM_CFLAGS,     -Wmissing-prototypes)
MC_CC_TRY_OPTION(AM_CFLAGS,     -Wno-pointer-sign)
MC_CC_TRY_OPTION(AM_CFLAGS,     -Wnested-externs)
MC_CC_TRY_OPTION(AM_CFLAGS,     -fno-cond-mismatch)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Wmissing-declarations)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Wpointer-arith)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Wredundant-decls)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -Werror=format-security)
MC_CXX_TRY_OPTION(AM_CXXFLAGS,  -Wsign-promo)
MC_CXX_TRY_OPTION(AM_CXXFLAGS,  -Woverloaded-virtual)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -rdynamic)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -pipe)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -O2)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -ftracer)
MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -mcx16)
dnl # MC_CXX_TRY_OPTION(AM_CXXFLAGS,  -Wno-invalid-offsetof)
MC_EVAR_ADD(AM_CFLAGS,   "$BOTHCCXXFLAGS")
MC_EVAR_ADD(AM_CXXFLAGS, "$BOTHCCXXFLAGS")
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(AM_CPPFLAGS)

# == CPU Optimizations ==
# MMX and SSE instruction sets have been introduced way more than a decade ago:
# Intel Pentium 3: CMOV MMX SSE (1999)
# VIA C3 Nehemiah: CMOV MMX SSE (2003)
# Athlon Palomino: CMOV MMX SSE 3DNow (2001)
SIMD_FLAGS=
MC_CC_TRY_OPTION(SIMD_FLAGS, -mmmx)
MC_CC_TRY_OPTION(SIMD_FLAGS, -msse)
# Note, -mfpmath=sse can harm FPU-algorithm performance
# Note, -funroll-loops can harm SSE loop performance
#MC_CC_TRY_OPTION(SIMD_FLAGS, -funroll-loops)
#AC_SUBST(SIMD_FLAGS)
# Expect the target architecture to support MMX/SSE if the compiler does it.
MC_EVAR_ADD(AM_CFLAGS,   "$SIMD_FLAGS")
MC_EVAR_ADD(AM_CXXFLAGS, "$SIMD_FLAGS")
# Newer vectorization instruction sets, should be optionally supported
# SSE >= 2, AVX
SSEAVX_FLAGS=
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse2)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse3)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -mssse3)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse4)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse4.1)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse4.2)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -msse4a)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -mavx)
MC_CC_TRY_OPTION(SSEAVX_FLAGS, -mavx2)
AC_SUBST(SSEAVX_FLAGS)

# == OPTIMIZE_FAST ==
# Some critical code paths should be optimized to run as fast as possible
OPTIMIZE_FAST=
MC_CC_TRY_OPTION(OPTIMIZE_FAST, -O3)
MC_CC_TRY_OPTION(OPTIMIZE_FAST, -ftree-vectorize)
MC_CC_TRY_OPTION(OPTIMIZE_FAST, -finline-functions)
MC_CC_TRY_OPTION(OPTIMIZE_FAST, -minline-all-stringops)
AC_SUBST(OPTIMIZE_FAST)

# == ENABLE_DEVEL_MODE ==
# check for development build rules, which require acess to .git/
AC_MSG_CHECKING(for ENABLE_DEVEL_MODE)
[test x"$enable_devel_mode" = xauto -a -d "$srcdir/.git/" && enable_devel_mode=yes]
test x"$enable_devel_mode" = xyes -o x"$enable_devel_mode" = x1 && enable_devel_mode=yes || enable_devel_mode=no
AC_MSG_RESULT([$enable_devel_mode])
edm=0; test x"$enable_devel_mode" = xyes && edm=1
AC_DEFINE_UNQUOTED(ENABLE_DEVEL_MODE, $edm, [Build a development version])
AM_CONDITIONAL(ENABLE_DEVEL_MODE, test x"$enable_devel_mode" = xyes)
if test x"$enable_devel_mode" = xyes ; then
  TEST_SRCDIR=$(cd $srcdir ; /bin/pwd)
  TEST_BUILDDIR=$(cd . ; /bin/pwd)
  test "$TEST_SRCDIR" == "$TEST_BUILDDIR" || {
    AC_MSG_ERROR([development build rules require srcdir == builddir])
  }
  git show HEAD:configure.ac 2>&1 | grep -q 'AC'_'INIT.*\brapicorn\b.*,' || {
    AC_MSG_ERROR([development build rules require a rapicorn.git repository])
  }
  DEVELMODE="" ; NOTDEVELMODE="#"
  # devel mode build tools
  MC_ASSERT_PROG(BISON, bison, [See: http://ftp.gnu.org/gnu/bison/])
  MC_ASSERT_PROG(FLEX, flex, [See: http://flex.sourceforge.net/])
  MC_ASSERT_PROG(PANDOC, pandoc, [See: http://johnmacfarlane.net/pandoc/])
  MC_ASSERT_PROG(DOXYGEN, doxygen, [See: http://www.stack.nl/~dimitri/doxygen/])
  MC_ASSERT_PROG(DOT, dot, [See: http://www.graphviz.org/])
  DEFINE__FILE_DIR__='-D__FILE_DIR__=\"${abs_srcdir}\"' # full debugging paths for devel-mode
else # provide sane defaults for meaningful makefiles & errors
  DEVELMODE="#"; NOTDEVELMODE=""
  BISON=bison; AC_SUBST(BISON)
  FLEX=flex; AC_SUBST(FLEX)
  PANDOC=pandoc; AC_SUBST(PANDOC)
  DOXYGEN=doxygen; AC_SUBST(DOXYGEN)
  DOT=dot; AC_SUBST(DOT)
  DEFINE__FILE_DIR__='-D__FILE_DIR__=\"rapicorn/${subdir}\"'
fi
AC_SUBST(DEVELMODE)
AC_SUBST(NOTDEVELMODE)
AC_SUBST(DEFINE__FILE_DIR__)

# C & C++ idioms
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDC

dnl # RAPICORN_SUBST_COMPUTED_SYSVAL(IDENTIFIER, INCLUDES) - defines & AC_SUBST()s RAPICORN_SYSVAL_$1
AC_DEFUN([RAPICORN_SUBST_COMPUTED_SYSVAL], 
    [AC_COMPUTE_INT([RAPICORN_SYSVAL_$1], [$1], [$2],
	    [AC_MSG_ERROR("failed to detect [$1]")])
    AC_SUBST([RAPICORN_SYSVAL_$1])])

dnl # RAPICORN_CXX_REQUIREMENTS() - check for required minimum __GNUC__ version
AC_DEFUN([RAPICORN_CXX_REQUIREMENTS],
[
    AC_MSG_CHECKING([for CXX compiler version])
    cxx_version=`$CXX --version 2>/dev/null | head -n1`
    AC_MSG_RESULT([$cxx_version])

    # check for minimum compiler version
    DEPVERSION_GNUC='4.7.4'
    DEPVERSION_CLANG='3.4.2'
    DEPVERSION_NR_GNUC=[`echo " $DEPVERSION_GNUC" | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)/(\1<<20)+(\2<<10)+\3/'`]
    DEPVERSION_NR_CLANG=[`echo " $DEPVERSION_CLANG" | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)/(\1<<20)+(\2<<10)+\3/'`]
    AC_MSG_CHECKING([for g++ >= $DEPVERSION_GNUC or clang++ >= $DEPVERSION_CLANG])
    AC_TRY_COMPILE([
	    #if defined __clang__
	    #  if (__clang_major__ << 20) + (__clang_minor__ << 10) + __clang_patchlevel__ < $DEPVERSION_NR_CLANG
	    #    error Clang version too old
	    #  endif
	    #elif defined __GNUC__
	    #  if (__GNUC__ << 20) + (__GNUC_MINOR__ << 10) + __GNUC_PATCHLEVEL__ < $DEPVERSION_NR_GNUC
	    #    error GCC version too old
	    #  endif
	    #endif
    ],[], depversion_gnuc_match=ok, depversion_gnuc_match=failed)
    AC_MSG_RESULT([$depversion_gnuc_match])
    if test "$depversion_gnuc_match" != "ok" ; then
	AC_MSG_ERROR([detected GNUC version too old])
    fi
])

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

    # --- POLL* ---
    poll_headers=["
      #define _GNU_SOURCE
      #include <sys/types.h>
      #include <sys/poll.h>
    "]
    AC_MSG_CHECKING([for POLL constants])
    RAPICORN_SUBST_COMPUTED_SYSVAL([POLLIN],     $poll_headers)
    RAPICORN_SUBST_COMPUTED_SYSVAL([POLLPRI],    $poll_headers)
    RAPICORN_SUBST_COMPUTED_SYSVAL([POLLOUT],    $poll_headers)
    RAPICORN_SUBST_COMPUTED_SYSVAL([POLLRDNORM], $poll_headers) # needs _GNU_SOURCE on linux
    RAPICORN_SUBST_COMPUTED_SYSVAL([POLLRDBAND], $poll_headers) # needs _GNU_SOURCE on linux
    RAPICORN_SUBST_COMPUTED_SYSVAL([POLLWRNORM], $poll_headers) # needs _GNU_SOURCE on linux
    RAPICORN_SUBST_COMPUTED_SYSVAL([POLLWRBAND], $poll_headers) # needs _GNU_SOURCE on linux
    RAPICORN_SUBST_COMPUTED_SYSVAL([POLLERR],    $poll_headers)
    RAPICORN_SUBST_COMPUTED_SYSVAL([POLLHUP],    $poll_headers)
    RAPICORN_SUBST_COMPUTED_SYSVAL([POLLNVAL],   $poll_headers)
    AC_MSG_RESULT([done])

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

    dnl # --- require libz ---
    if test -z "$LIBZ"; then
        AC_CHECK_LIB(z, gzsetparams,
            [AC_CHECK_HEADER(zlib.h, LIBZ='-lz', LIBZ='')],
            LIBZ='')
    fi
    if test -z "$LIBZ"; then
        AC_MSG_ERROR([Compression library libz is missing, but required])
    fi
    AC_SUBST(LIBZ)

    dnl --- require libpng ---
    if test -z "$LIBPNG"; then
	dnl # png_set_add_alpha() exists only since libpng v1.2
	AC_MSG_CHECKING(for libpng12)
	if $PKG_CONFIG --exists libpng12 ; then
	    AC_MSG_RESULT(yes)
	    LIBPNG=`$PKG_CONFIG --libs libpng12`
	elif $PKG_CONFIG --exists libpng13 ; then
	    AC_MSG_RESULT(yes)
	    LIBPNG=`$PKG_CONFIG --libs libpng13`
	else
	    AC_MSG_RESULT(no)
	    AC_MSG_ERROR([PNG image library libpng (>= 1.2) is missing, but required])
	fi
    fi

    dnl # --- check for GLib (version checked above) ---
    PKG_CHECK_MODULES(RAPICORN_CORE, glib-2.0 gio-2.0 gthread-2.0)
    # libtool doesn't automatically figure that -pthread implies -lpthread
    RAPICORN_CORE_LIBS=`echo " $RAPICORN_CORE_LIBS" | sed 's/\(-pthread\b\)/\1 -lpthread/' `
    RAPICORN_CORE_LIBS="$RAPICORN_CORE_LIBS $LIBPNG $LIBZ"
    AC_SUBST(RAPICORN_CORE_CFLAGS)
    AC_SUBST(RAPICORN_CORE_LIBS)
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

dnl # RAPICORN_REQUIREMENTS() - checks everything needed by ui/
AC_DEFUN([RAPICORN_REQUIREMENTS],
[
    dnl # --- check for Rapicorn GUI dependencies ---
    PKG_CHECK_MODULES(RAPICORN_GUI, cairo pangoft2 pangocairo)
    dnl # == X11 System ==
    AC_CHECK_LIB(X11, XOpenDisplay,
      [ RAPICORN_GUI_LIBS="$RAPICORN_GUI_LIBS -lX11" ],
      [ AC_MSG_ERROR([missing X11 library]) ])
    dnl # == X11 Extensions ==
    AC_CHECK_LIB(Xext, XShmAttach,
      [ RAPICORN_GUI_LIBS="$RAPICORN_GUI_LIBS -lXext" ],
      [ AC_MSG_ERROR([missing X11 shared memory extension]) ])
    RAPICORN_GUI_CFLAGS="$RAPICORN_GUI_CFLAGS $RAPICORN_CORE_CFLAGS"
    # libtool doesn't automatically figure that -pthread implies -lpthread
    RAPICORN_GUI_LIBS=`echo " $RAPICORN_GUI_LIBS" | sed 's/\(-pthread\b\)/\1 -lpthread/' `
    RAPICORN_GUI_LIBS="$RAPICORN_GUI_LIBS $RAPICORN_CORE_LIBS $LIBZ"
    AC_SUBST(RAPICORN_GUI_CFLAGS)
    AC_SUBST(RAPICORN_GUI_LIBS)
    AM_CONDITIONAL(WITH_PANGO, true)
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

dnl # RAPICORN_CYTHON_REQUIREMENTS() - rapicorn cython binding checks
AC_DEFUN([RAPICORN_CYTHON_REQUIREMENTS],
[
  # cython
  CYTHON_REQUIRED=0.20.1
  MC_ASSERT_PROG(CYTHON, cython, [cython can be found at http://cython.org/])
  CYTHON_VERSION=[$($CYTHON --version 2>&1 | sed 's/[^0-9]*\(.*\)$/\1/g')]
  AS_ECHO("checking cython version... $CYTHON_VERSION")
  MC_CHECK_VERSION([$CYTHON_VERSION], [$CYTHON_REQUIRED], [], [
      AC_MSG_ERROR([cython version $CYTHON_REQUIRED or newer is required])
  ])
  AC_SUBST(CYTHON)
  dnl # AM_CONDITIONAL([HAVE_CYTHON], [test "_$CYTHON" = "_cython"])
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

# Aida requirements
AC_DEFUN([RAPICORN_AIDA_REQUIREMENTS],
[
  # Python is checked for separately

  # XML linting check
  AC_CHECK_PROGS(XMLLINT, xmllint, true)
])

# check for package requirements.
MC_IF_VAR_EQ(GCC, yes,,
    AC_MSG_ERROR(This package requires GNU gcc)
)
MC_IF_VAR_EQ(GXX, yes,,
    AC_MSG_ERROR(This package requires GNU g++)
)

# find installation utilities
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database)

# check requirement sets
RAPICORN_I18N_REQUIREMENTS
RAPICORN_CXX_REQUIREMENTS
RAPICORN_CORE_REQUIREMENTS
RAPICORN_PYTHON_REQUIREMENTS
RAPICORN_CYTHON_REQUIREMENTS
RAPICORN_AIDA_REQUIREMENTS
RAPICORN_REQUIREMENTS
RAPICORN_TEST_REQUIREMENTS

# create output files
AC_CONFIG_FILES([
Makefile
po-helper.sh
po/Makefile.in
data/Makefile
rcore/Makefile
rcore/buildconfig.h
rcore/tests/Makefile
res/Makefile
aidacc/Makefile
aidacc/tests/Makefile
ui/Makefile
ui/tests/Makefile
cython/Makefile
tools/Makefile
tests/Makefile
pytests/Makefile
examples/Makefile
docs/Makefile
docs/mans/Makefile
docs/examples/Makefile
docs/imports/Makefile
docs/tutorial/Makefile
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
	Development mode:		${NOTDEVELMODE:+enabled}${DEVELMODE:+disabled}

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
