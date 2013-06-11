dnl # CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/

dnl # MC_DOXYGEN([doxygen-executables] [, dot-executables]) # check for working doxygen (>= 1.7.6.1) and dot
AC_DEFUN([MC_DOXYGEN], [
    AC_PATH_PROGS(DOXYGEN, [$1] doxygen)
    doxygen_version=`${DOXYGEN:-true} --version`
    AC_PATH_PROGS(DOT, [$2] dot)
    AC_MSG_CHECKING([for usable Doxygen])
    doxygen_error=""
    test -n "$DOXYGEN"		|| doxygen_error="failed to find doxygen"
    test -n "$DOT"		|| doxygen_error="failed to find dot"
    { echo "1.7.6.1" ; echo "$doxygen_version" ; } | sort -VC || doxygen_error="doxygen version too old"
    if test -z "$doxygen_error" ; then
      AC_MSG_RESULT([yes (have $doxygen_version and dot)])
    else
      AC_MSG_RESULT([no - $doxygen_error])
      AC_MSG_WARN([disabling Doxygen documentation builds])
      DOXYGEN=""
    fi
    AC_SUBST(DOXYGEN)
    AC_SUBST(DOT)
])
