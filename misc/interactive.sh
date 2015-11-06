#!/bin/bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# Execute $1 with args, and if any files change under PWD or subdirs re-execute $1

SCRIPTNAME=`basename $0` ; STARTPWD=`pwd`

export RAPICORN_FLIPPER=prevent-startup-focus # prevent focus stealing upon restarts

check() {
  find "$STARTPWD" -type f -printf '%T@ %p\n' | md5sum
}

EXIT=0
LAST=none
EXIT=0
PID=
while test $EXIT = 0 ; do
  CHECKSUM=`check`
  if test "$LAST" = "$CHECKSUM" ; then
    :;
  else
    test -z "$PID" || kill -15 "$PID"
    "$@" & PID=$!
    trap "kill -15 $PID" 0 HUP
    LAST="$CHECKSUM"
  fi
  sleep 1 ; EXIT=$?
  test -e /proc/$PID || { trap "" 0; exit 0 ; } # program ended
done
