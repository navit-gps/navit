#!/bin/bash -e
# ################################################################################################################### #
# This file exits 1 if there are files of interest that should trigger a build and exits normally otherwise.          #
# The idea is also to build if the exit code is different from 0 as it means we cannot get a filtered list properly.  #
# ################################################################################################################### #

# If we are on a tag, just exit 1 as we want to go on with the build
git describe --exact-match --tags HEAD 2>&1 2>/dev/null && exit 1 || echo "Not on a tag, checking files diff"

# This block constructs the list of files that differ from the trunk branch.
# Note that if you are on the trunk or master branch it will return the files modified by the last commit.
declare -a file_list=$(git diff --name-only refs/remotes/origin/trunk)
# If there is no diff that might just mean that you are on the trunk or master branch
# so you just want to check the last commit. This way we still have that check more
# or less working when pushing directly to trunk or when merging in master.
if [[ -z "$file_list" ]]; then
    file_list=$(git diff --name-only HEAD^)
fi

# This block filters out those don't match the pattern we use to exclude files that should not trigger a build.
declare -a filters=('^docs/.*' '.*\.md$' '.*\.rst$')
for f in ${file_list[@]}; do
    for filter in "${filters[@]}" ; do
        echo "checking $f with filter $filter"
        if [[ "$f" =~ $filter ]]; then
            # This removes the element from the element matching the filter
            file_list=(${file_list[@]/$f})
            echo "filtering out $f"
            break
        fi
    done
done

# exits with a 0 if the list is empty
[[ -z "${file_list}" ]]
