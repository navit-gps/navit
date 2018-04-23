#!/bin/bash
set -e

# ###########################################################################################################
# This script generates a AUTHORS file in the current directory based on the
# output of the git log command. It splits the contributors in 2 groups:
#  * The "active contributors" are the contributors that authored commits over the last 2 years
#  * The "retired contributors" are the contributors that have authored commits but not over the last 2 years
# ###########################################################################################################

# This map is used to map the old authors to the new ones to avoid duplicates
declare -A authorsMap=(
  ["Pierre GRANDIN"]="Pierre Grandin"
  ["kazer_"]="Pierre Grandin"
  ["Patrick Höhn"]="Patrick Höhn"
  ["Wildemann Stefan"]="Stefan Wildemann"
  ["pohlinkzei"]="Robert Pohlink"
  ["mdankov"]="Michael Dankov"
  ["sleske"]="Sebastian Leske"
  ["mcapdeville"]="Marc Capdeville"
  ["Marc CAPDEVILLE"]="Marc Capdeville"
  ["Gauthier60"]="gauthier60"
  ["horwitz"]="Michael Farmbauer"
  ["martin-s"]="Martin Schaller"
)
declare -A CONTRIBUTORS=()
declare -a authors=()
TWO_YEARS_AGO=`date +%s --date="2 years ago"`
retiredTitleWritten=false

git log --encoding=utf-8 --full-history --date=short "--format=format:%ad;%an" |
{
  echo "# Active contributors:" > AUTHORS
  while read -r line; do
    IFS=';'; arrLine=($line); unset IFS;
    author=${arrLine[1]}
    commitDate=`date +%s --date="${arrLine[0]}"`
  
    # Exclude circleci
    if [[ $author =~ [Cc]ircle\s*[Cc][Ii] ]]; then
      continue
    fi
  
    # indicates that the commits are now older than 2 years so we print the
    # sorted list of active contributors and reset the authors array
    if [ "$retiredTitleWritten" = false ]; then
      if [ $TWO_YEARS_AGO -ge $commitDate ]; then
        IFS=$'\n' sorted=($(sort <<<"${authors[*]}"))
        printf "%s\n" "${sorted[@]}" >> AUTHORS
        authors=()
        echo -e "\n# Retired contributors:" >> AUTHORS
        retiredTitleWritten=true
      fi
    fi
  
    # Remapping authors
    if [ -n "${authorsMap[${author}]}" ]; then
      author=${authorsMap[${author}]}
    fi
  
    # using the map as an easy way to check if the author has already been listed
    if [ -z "${CONTRIBUTORS[${author}]}" ]; then
      CONTRIBUTORS[${author}]=${arrLine[0]}
      authors+=("${author}")
    fi
  done
  # We are still in the subprocess scope so we can print the sorted list of
  # retired contributors
  IFS=$'\n' sorted=($(sort <<<"${authors[*]}"))
  printf "%s\n" "${sorted[@]}" >> AUTHORS
}
