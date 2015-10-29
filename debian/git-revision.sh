#!/bin/bash

set -e

SUFFIX=1
test "$#" -gt 0 && SUFFIX="$1"

# Require complete (unshallow) repository
test ! -s `git rev-parse --git-dir`/shallow || {
  echo "$0: fatal: complete commit count requires unshallow repository" >&2
  exit 77
  # fix: git fetch --unshallow
}

# Count commits to provide a monotonically increasing revision
TOTAL_COMMITS=`git rev-list --count HEAD`

DASH="" ; test -z "$SUFFIX" -o "${SUFFIX:0:1}" = '-' || DASH='-'

echo "+git$TOTAL_COMMITS$DASH$SUFFIX"
