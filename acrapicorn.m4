dnl # Rapicorn

dnl # MC_CHECK_VERSION() extracts up to 6 decimal numbers out of GIVEN_VERSION
dnl # and REQUIRED_VERSION, using any non-number letters as delimiters. It then
dnl # compares each of those 6 numbers in order 1..6 to each other, requirering
dnl # all of the 6 given-version numbers to be greater than, or at least equal
dnl # to the corresponding number of required-version.
dnl # MC_CHECK_VERSION( GIVEN_VERSION, REQUIRED_VERSION [, MATCH_ACTION] [, ELSE_ACTION] )
AC_DEFUN([MC_CHECK_VERSION], [
[eval `echo "$1:0:0:0:0:0:0" | sed -e 's/^[^0-9]*//' -e 's/[^0-9]\+/:/g' \
 -e 's/\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):\(.*\)/ac_v1=\1 ac_v2=\2 ac_v3=\3 ac_v4=\4 ac_v5=\5 ac_v6=\6/' \
`]
[eval `echo "$2:0:0:0:0:0:0" | sed -e 's/^[^0-9]*//' -e 's/[^0-9]\+/:/g' \
 -e 's/\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\):\(.*\)/ac_r1=\1 ac_r2=\2 ac_r3=\3 ac_r4=\4 ac_r5=\5 ac_r6=\6/' \
`]
ac_vm=[`expr \( $ac_v1 \> $ac_r1 \) \| \( \( $ac_v1 \= $ac_r1 \) \& \(		\
	      \( $ac_v2 \> $ac_r2 \) \| \( \( $ac_v2 \= $ac_r2 \) \& \(		\
	       \( $ac_v3 \> $ac_r3 \) \| \( \( $ac_v3 \= $ac_r3 \) \& \(	\
	        \( $ac_v4 \> $ac_r4 \) \| \( \( $ac_v4 \= $ac_r4 \) \& \(	\
	         \( $ac_v5 \> $ac_r5 \) \| \( \( $ac_v5 \= $ac_r5 \) \& \(	\
	          \( $ac_v6 \>= $ac_r6 \)					\
		 \) \)	\
		\) \)	\
	       \) \)	\
	      \) \)	\
	     \) \)	`]
case $ac_vm in
[1)]
	$3
	;;
*[)]
	$4
	;;
esac
])

dnl # MC_ASSERT_VERSION(versioncmd, requiredversion)
AC_DEFUN([MC_ASSERT_VERSION], [
	   ac_versionout=`( [$1] ) 2>&1 | tr '\n' ' ' | sed 's/^[^0-9]*//'`
	   MC_CHECK_VERSION([$ac_versionout], [$2], [], [
			      AC_MSG_ERROR([failed to detect version $2: $1])
			    ])
	 ])

dnl # MC_ASSERT_PROG(variable, programs)
AC_DEFUN([MC_ASSERT_PROG], [
	   AC_PATH_PROGS([$1], [$2], :)
	   case "_$[$1]" in
	     '_:')
	       AC_MSG_ERROR([failed to find program: $2])
	       ;;
	   esac
])
