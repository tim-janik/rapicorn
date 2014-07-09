dnl # Rapicorn                                       -*- Mode: shell-script; -*-
dnl # Copyright (C) 2005-2007 Tim Janik
dnl #
dnl # GNU Lesser General Public License version 2 or any later version.
dnl
builtin(include, acrapicorn.m4)dnl # include special macros
builtin(include, ld-symbolic.m4)dnl # include special macros

# Initialize autoconf for Rapicorn
AC_INIT([rapicorn],m4_esyscmd([
# Rapicorn version numbers
RAPICORN_RELEASE_YEAR=13     	# increment on ABI/API change
RAPICORN_RELEASE_MONTH=07    	# increment on ABI/API change
RAPICORN_REVISION_VERSION=1 	# ABI compatible increments, devel versions are odd
RAPICORN_RELEASE_CANDIDATE=	#rc1 # RAPICORN_RELEASE_CANDIDATE may contain rc[0-9]+ or nothing
RAPICORN_VERSION=$RAPICORN_RELEASE_YEAR.$RAPICORN_RELEASE_MONTH.$RAPICORN_REVISION_VERSION
echo -n $RAPICORN_VERSION${RAPICORN_RELEASE_CANDIDATE:+-}${RAPICORN_RELEASE_CANDIDATE}
])) # defines PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_STRING
AC_CONFIG_SRCDIR([ui/widget.hh])
AM_CONFIG_HEADER(configure.h)
AC_PREREQ(2.57)
# extract above versions
[RAPICORN_RELEASE_YEAR=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\1/'`]
[RAPICORN_RELEASE_MONTH=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\2/'`]
[RAPICORN_REVISION_VERSION=`echo $PACKAGE_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)\(.*\)/\3/'`]
[RAPICORN_RELEASE=`printf %02u%02u $RAPICORN_RELEASE_YEAR $(expr 0 + $RAPICORN_RELEASE_MONTH)`]
RAPICORN_VERSION=$RAPICORN_RELEASE_YEAR.$RAPICORN_RELEASE_MONTH.$RAPICORN_REVISION_VERSION
AC_SUBST(RAPICORN_RELEASE_YEAR)
AC_SUBST(RAPICORN_RELEASE_MONTH)
AC_SUBST(RAPICORN_REVISION_VERSION)
AC_SUBST(RAPICORN_RELEASE)
AC_SUBST(RAPICORN_VERSION)
AC_DEFINE_UNQUOTED(RAPICORN_VERSION, "$RAPICORN_VERSION", [Rapicorn Version])

# libtool versioning
LT_CURRENT=$RAPICORN_REVISION_VERSION
LT_REVISION=0
LT_AGE=$RAPICORN_REVISION_VERSION
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
if test 1 = $(($RAPICORN_REVISION_VERSION & 1)) ; then
  DEFINE__FILE_DIR__='-D__FILE_DIR__=\"${abs_srcdir}\"'
else
  DEFINE__FILE_DIR__='-D__FILE_DIR__=\"rapicorn/${subdir}\"'
fi
AC_SUBST(DEFINE__FILE_DIR__)

# Build tree checks
AC_MSG_CHECKING([for git repository])
test -f "${srcdir}/.git/index" && INGIT= || INGIT='#'
AC_SUBST(INGIT)
result=$(test -z "$INGIT" && echo yes || echo no)
AC_MSG_RESULT($result)

# Rapicorn namespace
AC_MSG_CHECKING([for Rapicorn namespace])
test -z "$RAPICORN_NAMESPACE_NAME" && RAPICORN_NAMESPACE_NAME="Rapicorn$RAPICORN_RELEASE"
xtmp=$(echo "$RAPICORN_NAMESPACE_NAME")  # expand shell variables
RAPICORN_NAMESPACE_NAME="$xtmp"
if test -z "$RAPICORN_NAMESPACE_NAME" || test "$RAPICORN_NAMESPACE_NAME" = "Rapicorn" ; then
    AC_MSG_RESULT(["$RAPICORN_NAMESPACE_NAME"])
    AC_MSG_ERROR([RAPICORN_NAMESPACE_NAME must be specified distinctly to build Rapicorn])
else
    AC_MSG_RESULT([$RAPICORN_NAMESPACE_NAME])
fi
AC_SUBST(RAPICORN_NAMESPACE_NAME)

# architecture information
AC_CANONICAL_TARGET
AC_DEFINE_UNQUOTED(RAPICORN_ARCH_NAME, "$target", [Architecture Description])

# initialize automake, don't define PACKAGE and VERSION
AM_INIT_AUTOMAKE(tar-pax no-define)
AC_PROG_MAKE_SET        dnl # support subdirectories.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# declare --enable-* args and collect ac_help strings
AC_ARG_ENABLE(debug,       [  --enable-debug=[no/minimum/yes] turn on debugging [default=yes]],,enable_debug=yes)
# package library defaults
enable_static=no ; enable_shared=yes

# configure requirements
MC_ASSERT_PROG(PKG_CONFIG, pkg-config, [pkg-config can be found at http://pkgconfig.freedesktop.org/])

# check for base package requirements early on
PKG_CHECK_MODULES(CAIRO, [
	cairo		>= 1.10.0
])
PKG_CHECK_MODULES(BASE_DEPENDENCIES, [
	cairo		>= 1.10.0
	glib-2.0	>= 2.32.0
	gio-2.0		>= 2.30.0
	gthread-2.0	>= 2.30.0
	pango		>= 1.29.0
	pangoft2	>= 1.29.0
	pangocairo	>= 1.29.0
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
ACX_PROG_LD_GNU_SYMBOLIC
AC_SUBST([SYMBOLIC_LDFLAGS])
ACX_PROG_LD_GNU_DYNAMIC_LIST_CPP_NEW
AC_SUBST([DYNAMIC_LIST_CPP_NEW_LDFLAGS])
AM_PROG_LIBTOOL
AC_LIBTOOL_DLOPEN

# AM_LDFLAGS, optimize linker to use faster ELF symbol hashing if we have compiler and linker support
AC_MSG_CHECKING([whether CC and LD support gnu.hash sections])
if ${CC-cc} -dumpspecs | grep -q -- '--hash-style=\(both\|gnu\).*--as-needed' && \
   ${LD-ld} --help     | grep -q -- '--hash-style=.*\(both\|gnu\)' ; then
  MC_EVAR_ADD(AM_LDFLAGS, [-Wl,--hash-style=both -Wl,--as-needed -Wl,-O1])
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
MC_EVAR_ADD(AM_LDFLAGS, -pthread)
MC_EVAR_ADD(AM_LDFLAGS, -lrt)
AC_SUBST(AM_LDFLAGS)

# Pick C++ dialect, needed in CXXFLAGS for compile tests
MC_CXX_TRY_OPTION(CXXFLAGS,  -std=gnu++0x)

# AM_CFLAGS & AM_CXXFLAGS
MC_CXX_TRY_OPTION(AM_CXXFLAGS,  -std=gnu++0x)
BOTHCCXXFLAGS=
if test "x$enable_debug" = "xyes"; then
  MC_EVAR_ADD(BOTHCCXXFLAGS,      -DG_ENABLE_DEBUG)
  MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -g)
  MC_CC_TRY_OPTION(BOTHCCXXFLAGS, -ggdb3)
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
# Expect the target architecture to support MMX/SSE if the cmpiler does it.
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

# C & C++ idioms
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDC

dnl # RAPICORN_SUBST_COMPUTED_SYSVAL(IDENTIFIER, INCLUDES) - defines & AC_SUBST()s RAPICORN_SYSVAL_$1
AC_DEFUN([RAPICORN_SUBST_COMPUTED_SYSVAL], 
    [AC_COMPUTE_INT([RAPICORN_SYSVAL_$1], [$1], [$2],
	    [AC_MSG_ERROR("failed to detect [$1]")])
    AC_SUBST([RAPICORN_SYSVAL_$1])])

dnl # RAPICORN_GCC_REQUIREMENTS() - check for required minimum GCC version
AC_DEFUN([RAPICORN_GCC_REQUIREMENTS],
[
    # check for minimum GCC version
    DEPVERSION_GNUC='4.2.4'
    DEPVERSION_FORMULA=[`echo " $DEPVERSION_GNUC" | sed 's/\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)/(\1<<20)+(\2<<10)+\3/'`]
    AC_MSG_CHECKING([for minimum GCC version ($DEPVERSION_GNUC)])
    AC_TRY_COMPILE([
	    #if defined __GNUC__
	    #  if (__GNUC__ << 20) + (__GNUC_MINOR__ << 10) + __GNUC_PATCHLEVEL__ < $DEPVERSION_FORMULA
	    #    error GCC version too old
	    #  endif
	    #endif
    ],[], depversion_gnuc_match=ok, depversion_gnuc_match=failed)
    AC_MSG_RESULT([$depversion_gnuc_match])
    if test "$depversion_gnuc_match" != "ok" ; then
	AC_MSG_ERROR([detected GNUC version too old])
    fi
])

dnl # RAPICORN_RSVG_REQUIREMENTS() - checks everything needed for embedding librsvg
AC_DEFUN([RAPICORN_RSVG_REQUIREMENTS],
[
    AC_CHECK_FUNCS(strtok_r)
    PKG_CHECK_MODULES(LIBXML,		[ libxml-2.0		>= 2.4.7  ])
    PKG_CHECK_MODULES(CAIRO,		[ cairo			>= 1.2.4  ])
    PKG_CHECK_MODULES(GDKPIXBUF,	[ gdk-pixbuf-2.0	>= 2.22.0 ])
    PKG_CHECK_MODULES(PANGO,		[ pango			>= 1.14.8 ])
    PKG_CHECK_MODULES(PANGOCAIRO,	[ pangocairo		>= 1.10.0 ])
    RAPICORN_RSVG_CFLAGS="$PANGO_CFLAGS $GDKPIXBUF_CFLAGS $LIBXML_CFLAGS $CAIRO_CFLAGS"
    RAPICORN_RSVG_LIBS="$PANGO_LIBS $GDKPIXBUF_LIBS $LIBXML_LIBS $CAIRO_LIBS -lm"
    AC_SUBST(RAPICORN_RSVG_CFLAGS)
    AC_SUBST(RAPICORN_RSVG_LIBS)

])

dnl # RAPICORN_CORE_REQUIREMENTS() - checks everything needed for rapicornconfig.h
AC_DEFUN([RAPICORN_CORE_REQUIREMENTS],
[
    # --- Rapicorn paths ---
    docdir='${datarootdir}/doc/rapicorn'"$RAPICORN_RELEASE"	# docdir='${datarootdir}/doc/${PACKAGE}'
    rapicorndatadir='${datadir}/rapicorn'"$RAPICORN_RELEASE"	# datadir = ${datarootdir}
    rapicornsvgdir='${rapicorndatadir}/svg'
    AC_SUBST(docdir)
    AC_SUBST(rapicorndatadir)
    AC_SUBST(rapicornsvgdir)

    # --- sizeof and presence of uint ---
    GLIB_SIZEOF([#include <sys/types.h>], uint, rapicorn_sys_typesh_uint)
    RAPICORN_SIZEOF_SYS_TYPESH_UINT="$glib_cv_sizeof_rapicorn_sys_typesh_uint"
    AC_SUBST(RAPICORN_SIZEOF_SYS_TYPESH_UINT) dnl # for rapicornconfig.h
    GLIB_SIZEOF([], void**, rapicorn_pointer)
    RAPICORN_SIZEOF_POINTER="$glib_cv_sizeof_rapicorn_pointer"
    AC_SUBST(RAPICORN_SIZEOF_POINTER) dnl # for rapicornconfig.h
    GLIB_SIZEOF([], long, rapicorn_long)
    RAPICORN_SIZEOF_LONG="$glib_cv_sizeof_rapicorn_long"
    AC_SUBST(RAPICORN_SIZEOF_LONG) dnl # for rapicornconfig.h

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
    AC_SUBST(RAPICORN_SIZEOF_PTHREADH_RWLOCK) dnl # for rapicornconfig.h

    # == sizeof pthread_spinlock_t ==
    GLIB_SIZEOF([#include <pthread.h>], pthread_spinlock_t, rapicorn_pthreadh_spinlock)
    RAPICORN_SIZEOF_PTHREADH_SPINLOCK="$glib_cv_sizeof_rapicorn_pthreadh_spinlock"
    AC_SUBST(RAPICORN_SIZEOF_PTHREADH_SPINLOCK) dnl # for rapicornconfig.h

    # == pthread_spinlock_t initializer ==
    MC_DEFINE_SPINLOCK_INITIALIZER()
    RAPICORN_SPINLOCK_INITIALIZER="$mc_cv_spinlock_initializer"
    AC_SUBST(RAPICORN_SPINLOCK_INITIALIZER) dnl # for rapicornconfig.h

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
      AC_MSG_CHECKING([whether g++<=4.7 sleep_for/yield workaround is needed])
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
    RAPICORN_GETTEXT_DOMAIN=rapicorn$RAPICORN_RELEASE    # version without -rcZ
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
    RAPICORN_GUI_LIBS="$RAPICORN_GUI_LIBS $RAPICORN_CORE_LIBS $RAPICORN_RSVG_LIBS $LIBZ"
    AC_SUBST(RAPICORN_GUI_CFLAGS)
    AC_SUBST(RAPICORN_GUI_LIBS)
    AM_CONDITIONAL(WITH_PANGO, true)
    # --- Rapicorn build tools ---
    MC_ASSERT_PROG(BISON, bison, [See: http://ftp.gnu.org/gnu/bison/])
    MC_ASSERT_PROG(FLEX, flex, [See: http://flex.sourceforge.net/])
])

dnl # RAPICORN_ROPE_REQUIREMENTS() - rapicorn python binding checks
AC_DEFUN([RAPICORN_ROPE_REQUIREMENTS],
[
    # Find Python execution and development environment
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="-Wall -Werror $CPPFLAGS" # ensure sane Python module builds
    mypyversion=2.4
    AM_PATH_PYTHON($mypyversion)
    AC_REQUIRE([AC_PYTHON_DEVEL]) # AC_PYTHON_DEVEL is usually provided by autoconf-archive
    AC_PYTHON_DEVEL([>= '$mypyversion'])
    CPPFLAGS="$CPPFLAGS $PYTHON_CPPFLAGS"
    AC_MSG_CHECKING([for working Python extension compilation])
    PYTHON_CPPFLAGS="$PYTHON_CPPFLAGS -fno-strict-aliasing" # needed by Python headers
    AC_TRY_COMPILE([#include <Python.h>], [Py_InitModule(0,0);], py_build=ok, py_build=failed)
    AC_MSG_RESULT($py_build)
    if test "$py_build" != "ok" ; then
	AC_MSG_ERROR([failed to build extension against Python (>=$DEPVERSION_PYTHON) headers])
    fi
    CPPFLAGS="$saved_CPPFLAGS"
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
    # Check for working $PYTHON and pythondir
    dnl #ALREADYDONE# AM_PATH_PYTHON(2.4)
    AC_MSG_CHECKING([for usable Python])
    if $PYTHON -c "import sys; sys.exit (not sys.version_info >= (2,4))" &&
       test -n "$pythondir" ; then
      AC_MSG_RESULT(ok)
    else
      AC_MSG_RESULT(fail)
      AC_MSG_ERROR([failed to check \$PYTHON and \$pythondir])
    fi

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

# check for development tools
AC_CHECK_PROGS(XMLLINT, xmllint, true)

# find installation utilities
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database)

# check requirement sets
RAPICORN_I18N_REQUIREMENTS
RAPICORN_GCC_REQUIREMENTS
RAPICORN_CORE_REQUIREMENTS
RAPICORN_ROPE_REQUIREMENTS
RAPICORN_AIDA_REQUIREMENTS
RAPICORN_RSVG_REQUIREMENTS
RAPICORN_REQUIREMENTS
RAPICORN_TEST_REQUIREMENTS

# create output files
AC_CONFIG_FILES([
Makefile
po-helper.sh
po/Makefile.in
data/Makefile
rcore/Makefile
rcore/rsvg/Makefile
rcore/tests/Makefile
res/Makefile
aidacc/Makefile
aidacc/tests/Makefile
ui/Makefile
ui/tests/Makefile
rope/Makefile
tools/Makefile
tests/Makefile
pytests/Makefile
examples/Makefile
docs/Makefile
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
	C++ Namespace:			$RAPICORN_NAMESPACE_NAME
	Operating System:		$RAPICORN_OS

	CPU optimizations:		${FPU_FLAGS:+fpu}${MMX_FLAGS:+ mmx}${AMD3DNOW_FLAGS:+ 3dnow}${SSE_FLAGS:+ sse}${SSE2_FLAGS:+ sse2}${SSE3_FLAGS:+ sse3}

	Python extension:		enabled (curently mandatory)
	Readline libs:			$READLINELIBS

	Binaries:			`printexpandvar ${bindir}`
	Libraries:			`printexpandvar ${libdir}`
	Locales:			`printexpandvar ${pkglocaledir}`
	Includes:			`printexpandvar ${includedir}/rapicorn$RAPICORN_RELEASE`
	SVG-Library:			`printexpandvar ${rapicornsvgdir}`
	Data:				`printexpandvar ${datadir}`
	Documents:			`printexpandvar ${docdir}`
	PyRapicorn:			`printexpandvar ${pythondir}/$RAPICORN_NAMESPACE_NAME`

EOF
