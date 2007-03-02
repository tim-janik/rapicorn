dnl # Birnet                                       -*- Mode: shell-script; -*-
dnl # Copyright (C) 2006 Tim Janik
dnl #
dnl # GNU Lesser General Public License version 2 or any later version.

dnl # BIRNET_COMPUTE_SYSVAL(IDENTIFIER, INCLUDES) defines & AC_SUBST()s BIRNET_SYSVAL_$1
AC_DEFUN([BIRNET_SUBST_COMPUTED_SYSVAL], 
    [_AC_COMPUTE_INT([$1], [BIRNET_SYSVAL_$1], [$2],
	    [AC_MSG_ERROR("failed to detect [$1]")])
    AC_SUBST([BIRNET_SYSVAL_$1])])

dnl # AC_BIRNET_REQUIREMENTS() checks everything needed for birnetconfig.h
AC_DEFUN([AC_BIRNET_REQUIREMENTS],
[
    # --- Birnet version numbers ---
    AC_DIVERT_BEFORE_HELP([
    BIRNET_RELEASE_CHECKSUM=95fe2f2d9b109503ae6e83f7fd8c9454
    BIRNET_RELEASE_VERSION=702  # increment when ABI or API changes
    BIRNET_REVISION_VERSION=0   # increment while ABI compatible, otherwise reset
    BIRNET_RELEASE_CANDIDATE=   # rc[0-9]+
    BIRNET_VERSION=$BIRNET_RELEASE_VERSION.$BIRNET_REVISION_VERSION
    BIRNET_PACKAGE_VERSION=$BIRNET_VERSION # -rcZ to be appended...
    test -n "$BIRNET_RELEASE_CANDIDATE" && BIRNET_PACKAGE_VERSION=$BIRNET_VERSION-$BIRNET_RELEASE_CANDIDATE
    ])
    AC_SUBST(BIRNET_RELEASE_CHECKSUM)
    AC_SUBST(BIRNET_RELEASE_VERSION)
    AC_SUBST(BIRNET_REVISION_VERSION)
    AC_SUBST(BIRNET_VERSION)
    AC_SUBST(BIRNET_PACKAGE_VERSION)
    AC_DEFINE_UNQUOTED(BIRNET_VERSION, "$BIRNET_VERSION", [Birnet Version])
    AC_DEFINE_UNQUOTED(BIRNET_PACKAGE_VERSION, "$BIRNET_PACKAGE_VERSION", [Birnet Package/Tarball Version])
    BIRNET_SKIP_VERSION_CHECK='#'
    grep -q '^AM''_INIT_AUTOMAKE(birnet,' ./configure.in && BIRNET_SKIP_VERSION_CHECK=''
    AC_SUBST(BIRNET_SKIP_VERSION_CHECK)

    # --- Birnet namespace ---
    AC_MSG_CHECKING([for Birnet namespace])
    xtmp=$(echo "$BIRNET_NAMESPACE_NAME")  # expand shell variables
    BIRNET_NAMESPACE_NAME="$xtmp"
    if test -z "$BIRNET_NAMESPACE_NAME" || test "$BIRNET_NAMESPACE_NAME" = "Birnet" ; then
	    AC_MSG_RESULT(["$BIRNET_NAMESPACE_NAME"])
	    AC_MSG_ERROR([BIRNET_NAMESPACE_NAME must be specified distinctly to build Birnet])
    else
	    AC_MSG_RESULT([$BIRNET_NAMESPACE_NAME])
    fi
    AC_SUBST(BIRNET_NAMESPACE_NAME)

    # --- sizeof and presence of uint ---
    GLIB_SIZEOF([#include <sys/types.h>], uint, birnet_sys_typesh_uint)
    BIRNET_SIZEOF_SYS_TYPESH_UINT="$glib_cv_sizeof_birnet_sys_typesh_uint"
    AC_SUBST(BIRNET_SIZEOF_SYS_TYPESH_UINT) dnl # for birnetconfig.h
    GLIB_SIZEOF([], void**, birnet_pointer)
    BIRNET_SIZEOF_POINTER="$glib_cv_sizeof_birnet_pointer"
    AC_SUBST(BIRNET_SIZEOF_POINTER) dnl # for birnetconfig.h
    GLIB_SIZEOF([], long, birnet_long)
    BIRNET_SIZEOF_LONG="$glib_cv_sizeof_birnet_long"
    AC_SUBST(BIRNET_SIZEOF_LONG) dnl # for birnetconfig.h

    # --- POLL* ---
    poll_headers=["
      #define _GNU_SOURCE
      #include <sys/types.h>
      #include <sys/poll.h>
    "]
    AC_MSG_CHECKING([for POLL constants])
    BIRNET_SUBST_COMPUTED_SYSVAL([POLLIN],     $poll_headers)
    BIRNET_SUBST_COMPUTED_SYSVAL([POLLPRI],    $poll_headers)
    BIRNET_SUBST_COMPUTED_SYSVAL([POLLOUT],    $poll_headers)
    BIRNET_SUBST_COMPUTED_SYSVAL([POLLRDNORM], $poll_headers) # needs _GNU_SOURCE on linux
    BIRNET_SUBST_COMPUTED_SYSVAL([POLLRDBAND], $poll_headers) # needs _GNU_SOURCE on linux
    BIRNET_SUBST_COMPUTED_SYSVAL([POLLWRNORM], $poll_headers) # needs _GNU_SOURCE on linux
    BIRNET_SUBST_COMPUTED_SYSVAL([POLLWRBAND], $poll_headers) # needs _GNU_SOURCE on linux
    BIRNET_SUBST_COMPUTED_SYSVAL([POLLERR],    $poll_headers)
    BIRNET_SUBST_COMPUTED_SYSVAL([POLLHUP],    $poll_headers)
    BIRNET_SUBST_COMPUTED_SYSVAL([POLLNVAL],   $poll_headers)
    AC_MSG_RESULT([done])

    # --- OS/Win32 detection ---
    dnl # needs AC_CANONICAL_HOST
    test -z "$host" && {
	missing_macro="AC""_CANONICAL_HOST"
	AC_MSG_ERROR([configure failed to execute $missing_macro])
    }
    AC_MSG_CHECKING([for Win32])
    BIRNET_OS_CHOICES="WIN32 UNIX" # OS types we will check for
    case "$host" in
	*-*-mingw*)
	    BIRNET_OS=WIN32
	    AC_MSG_RESULT([yes])
	    ;;
	*)
	    BIRNET_OS=UNIX
	    AC_MSG_RESULT([no])
	    ;;
    esac
    AC_SUBST(BIRNET_OS)
    AC_SUBST(BIRNET_OS_CHOICES)
    AC_DEFINE_UNQUOTED(BIRNET_OS_$BIRNET_OS, "1", [Win32 detection])

    dnl # --- sizeof threading structs ---
    GLIB_SIZEOF([#include <pthread.h>], pthread_mutex_t, pth_mutex_t)
    BIRNET_SIZEOF_PTH_MUTEX_T="$glib_cv_sizeof_pth_mutex_t"
    AC_SUBST(BIRNET_SIZEOF_PTH_MUTEX_T)
    GLIB_SIZEOF([#include <pthread.h>], pthread_cond_t, pth_cond_t)
    BIRNET_SIZEOF_PTH_COND_T="$glib_cv_sizeof_pth_cond_t"
    AC_SUBST(BIRNET_SIZEOF_PTH_COND_T)

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
      BIRNET_HAVE_MUTEXATTR_SETTYPE=1
      AC_MSG_RESULT(yes)
    ],[
      BIRNET_HAVE_MUTEXATTR_SETTYPE=0
      AC_MSG_RESULT(no)
    ])
    AC_SUBST(BIRNET_HAVE_MUTEXATTR_SETTYPE)

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

    dnl # --- check for GLib --- 
    DEPENDANCIES="glib-2.0 >= 2.6.4 gthread-2.0"
    dnl # define BIRNET_GLIB_CFLAGS and BIRNET_GLIB_LIBS:
    PKG_CHECK_MODULES(BIRNET_GLIB, $DEPENDANCIES,,
	AC_MSG_ERROR([[Failed to detect or find $DEPENDANCIES (consider adjusting \$PKG_CONFIG_PATH)]]))
    dnl # libtool doesn't automatically figure that -pthread implies -lpthread
    BIRNET_LIBS=`echo " $BIRNET_GLIB_LIBS" | sed 's/\(-pthread\b\)/-lpthread \1/' `
    BIRNET_LIBS="$BIRNET_LIBS $LIBZ"
    BIRNET_CFLAGS="$BIRNET_GLIB_CFLAGS"

    dnl # --- check for memory barriers (needs glib) ---
    AC_MSG_CHECKING([for memory barriers])
    saved_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $BIRNET_CFLAGS"
    saved_LIBS="$LIBS"
    LIBS="$LIBS $BIRNET_LIBS"
    AC_TRY_RUN([ #include <glib.h>
	    int main () {
	    #if defined (G_ATOMIC_OP_MEMORY_BARRIER_NEEDED) || (!GLIB_CHECK_VERSION (2, 8, 0) && !defined (__i386__))
		return 1;
	    #else
		return 0;
	    #endif
	    }],
	BIRNET_MEMORY_BARRIER_NEEDED=0
	AC_MSG_RESULT(none needed)
        ,
	BIRNET_MEMORY_BARRIER_NEEDED=1
	AC_MSG_RESULT(required)
        ,)
    CFLAGS="$saved_CFLAGS"
    LIBS="$saved_LIBS"
    AC_SUBST(BIRNET_MEMORY_BARRIER_NEEDED)

    dnl # --- BIRNET_CFLAGS & BIRNET_LIBS ---
    AC_SUBST(BIRNET_CFLAGS)
    AC_SUBST(BIRNET_LIBS)
])
