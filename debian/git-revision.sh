#!/bin/bash

# Print current git history commit count, according to FORMAT, substitutions:
# %%	percent sign
# %r	total commit count

set -e

FORMAT="+git%r-1"
test "$#" -le 0 || FORMAT="$1"

# Require complete (unshallow) repository
test ! -s `git rev-parse --git-dir`/shallow || {
  echo "$0: fatal: complete commit count requires unshallow repository" >&2
  exit 77
  # upgrade repo: git fetch --unshallow
}

# Count commits to provide a monotonically increasing revision
TOTAL_COMMITS=`git rev-list --count HEAD`
X='_0SygirT1z_' # temporary replacement pattern, unlikely to occour in formats

echo "_$FORMAT" | sed "s/^_// ; s/%\(.\)/$X\1/g; s/$X""r/$TOTAL_COMMITS/g; s/$X%/%/g"
