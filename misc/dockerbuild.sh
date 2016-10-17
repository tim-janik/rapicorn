#!/bin/bash
set -e

# setup
SCRIPTNAME="$(basename "$0")" ; die() { e="$1"; shift; echo "$SCRIPTNAME: $*" >&2; exit "$e"; }
SCRIPTDIR="$(dirname "$(readlink -f "$0")")"

# parse args
test $# -gt 0 || die 1 "Usage: dockerbuild.sh <DIST> [INTENT]"
DIST=debian:jessie
INTENT=distcheck
test $# -lt 1 || { DIST="$1"; shift; }
test $# -lt 1 || { INTENT="$1"; shift; }
DISTRELEASE="${DIST#*:}"

# provide a full rapicorn.git clone (which might be a worktree pointer)
rm -rf ./tmp-mirror.git/
git clone --mirror .git tmp-mirror.git

# configure Dockerfile
export DIST INTENT DISTRELEASE TRAVIS_JOB_NUMBER http_proxy
misc/applyenv.sh misc/Dockerfile-apt.in > Dockerfile

# build Rapicorn in docker
docker build -t rapicorn .
echo -e "OK, EXAMINE:\n  docker run -ti --rm rapicorn /bin/bash"

# cleanup
rm -rf ./tmp-mirror.git/
