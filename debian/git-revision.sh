#!/bin/bash

set -ex

# Ensure we're in the Rapicorn repository
git rev-list --quiet bfe04a96a143db73181805241f5df28330e175aa

# Count commits to provide a monotonically increasing revision
git rev-list --count HEAD bfe04a96a143db73181805241f5df28330e175aa
