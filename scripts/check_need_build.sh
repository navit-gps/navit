#!/bin/bash -e
# ################################################################################################################### #
# This file exits 1 if there are files of interest that should trigger a build and exits normally otherwise.          #
# The idea is also to build if the exit code is different from 0 as it means we cannot get a filtered list properly.  #
# ################################################################################################################### #

# @brief: constructs the list of files that differ from the trunk branch.
#         Note that if you are on the trunk or master branch it will return the files modified by the last commit.
# @param: a variable you want to use to get the resulting value
# @return: nothing (the input variable is modified according to the brief description)
list_files_git_diff(){
    local -n files=$1
    files=$(git diff --name-only refs/remotes/origin/trunk)

    # If there is no diff that might just mean that you are on the trunk or master branch
    # so you just want to check the last commit. This way we still have that check more
    # or less working when pushing directly to trunk or when merging in master.
    if [[ -z "$files" ]]; then
        files=$(git diff --name-only HEAD^)
    fi
}

# @brief: get the list of files that have differed from trunk (by calling list_files_git_diff for the full list)
#         and filters out those don't match the pattern we use to exclude files that should not trigger a build.
# @param: a variable you want to use to get the resulting value
# @return: nothing (the input variable is modified according to the brief description)
filtered_files_git_diff(){
    local -n _ret=$1
    declare -a filters=('^docs/.*', '.*\.md$', '.*\.rst$')
    declare -a file_list=()
    list_files_git_diff file_list
    for f in ${file_list[@]}; do
        for filter in "${filters[@]}" ; do
            if [[ "$f" =~ $filter ]]; then
                # This removes the element from the element matching the filter
                file_list=(${file_list[@]/$f})
                break
            fi
        done
    done
    _ret=$file_list
}

filtered_files_git_diff filtered_out_modified_files
# exits with a 0 if the list is empty
[[ -z "${filtered_out_modified_files}" ]]
