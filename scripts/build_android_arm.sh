#!/bin/bash
# Build Navit for Android.
#
# This script is to be run from the root of the Navit source tree.
#
# It is intended for compatibility and will simply run build_android.sh.
#
# When writing or maintaining code, do not call this script but call build_android.sh directly, as this
# script will eventually be removed.

echo WARNING: This script is deprecated, use build_android.sh instead!

./scripts/build_android.sh
