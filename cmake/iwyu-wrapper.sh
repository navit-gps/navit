#!/bin/sh
# First argument is the path to include-what-you-use, the rest are the
# compiler arguments. include-what-you-use forwards all arguments to its
# bundled clang driver, which rejects the GCC-only -fanalyzer flag used on
# Debug builds, so strip it before invoking include-what-you-use.
set -e

iwyu="$1"
shift

n=$#
while [ "$n" -gt 0 ]; do
	arg=$1
	shift
	case "$arg" in
		-fanalyzer) ;;
		*) set -- "$@" "$arg" ;;
	esac
	n=$((n - 1))
done

exec "$iwyu" "$@"