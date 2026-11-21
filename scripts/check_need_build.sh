#!/bin/bash -e
# ################################################################################################################### #
# This file exits 1 if there are files of interest that should trigger a build and exits normally otherwise.          #
# The idea is also to build if the exit code is different from 0 as it means we cannot get a filtered list properly.  #
# ################################################################################################################### #

# If we are on a tag, just exit 1 as we want to go on with the build
git describe --exact-match --tags HEAD 2>&1 2>/dev/null && exit 1 || echo "Not on a tag, checking files diff"

source "$(dirname "$0")/ci_base.sh"

if ! mergebase=$(resolve_mergebase); then
    echo "[ERROR] No merge base found; cannot determine changed files, building." >&2
    exit 1
fi

# List the files that differ from the fork point of the branch.
declare -a diff_files=()
while IFS= read -r -d '' f; do
    diff_files+=("$f")
done < <(git diff --name-only -z "$mergebase")

# If there is no diff that might just mean that you are on the trunk or master branch
# so you just want to check the last commit. This way we still have that check more
# or less working when pushing directly to trunk or when merging in master.
if [[ ${#diff_files[@]} -eq 0 ]]; then
    while IFS= read -r -d '' f; do
        diff_files+=("$f")
    done < <(git diff --name-only -z HEAD^)
fi

# This block filters out those don't match the pattern we use to exclude files that should not trigger a build.
filters=('^docs/.*' '.*\.md$' '.*\.rst$')
declare -a build_files=()
for f in "${diff_files[@]}"; do
    keep=true
    for filter in "${filters[@]}"; do
        echo "checking $f with filter $filter"
        if [[ "$f" =~ $filter ]]; then
            echo "filtering out $f"
            keep=false
            break
        fi
    done
    if $keep; then
        build_files+=("$f")
    fi
done

# exits with a 0 if no build-relevant file changed
[[ ${#build_files[@]} -eq 0 ]]
