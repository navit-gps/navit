#!/bin/bash
set -e

# ###########################################################################################################
# This script generates a AUTHORS file in the current directory based on the
# output of the git log command. It splits the contributors in 2 groups:
#  * The "active contributors" are the contributors that authored commits over the last 2 years
#  * The "retired contributors" are the contributors that have authored commits but not over the last 2 years
# Note: it uses git's mailmap functionnality to get a clean list of users
# ###########################################################################################################

recentcontributors=$(git log --after="2 years ago" --encoding=utf-8 --full-history --date=short --use-mailmap "--format=format:%aN"  | sort  -fu)
allcontributors=$(git log  --encoding=utf-8 --full-history --date=short --use-mailmap "--format=format:%aN"|sort -fu)


echo "# Active contributors:" > AUTHORS
echo "$recentcontributors" >> AUTHORS
echo -e "\n# Retired contributors:" >> AUTHORS
echo -e "$recentcontributors\n$allcontributors" |sort | uniq -u >> AUTHORS
