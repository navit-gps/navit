#!/usr/bin/env bash
set -x
set -eu

# #############################################################################
# This script runs on circle CI to verify that the code of the PR has been
# sanitized before push.
# #############################################################################

# List the files that are different from the trunk
from=$(git rev-parse refs/remotes/origin/trunk)
to=$(git rev-parse HEAD)
interval=${from}..${to}
[[ "${from}" == "${to}" ]] && interval=${to}

for f in $(git show -m --pretty="format:" --name-only ${interval}); do
  if [[ -e "${f}" ]]; then
    echo $f
    if [[ "$f" != "*.bat" ]]; then
      # Removes trailing spaces
      [[ "$(file -bi """${f}""")" =~ ^text ]] && sed 's/\s*$//' -i "${f}"
    fi
    # Formats any *.c and *.cpp files
    if [[ "$f" == "*.c" ]] || [[ "$f" == "*.cpp" ]]; then
      astyle --indent=spaces=4 --style=attach -n --max-code-length=120 -xf -xh "${f}"
    fi
  fi
done

# Check if any file has been modified. If yes, that means the best practices
# have not been followed, so we fail the job.
git diff --exit-code
code=$?
if [[ $code -ne 0 ]]; then
  echo "You may need to do some cleanup in the files you commited, see the git diff output above."
fi
git checkout -- .
exit $code
