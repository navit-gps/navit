#!/usr/bin/env bash
set -u

# #####################################################################################################################
# This script runs on circle CI to verify that the code of the PR has been
# sanitized before push.
#
# WARNING: make sure you commit ALL your changes before running it locally if you ever do it because it will run a git
# checkout -- which will reset your changes on all files...
# #####################################################################################################################

return_code=0

# Check if any file has been modified. If yes, that means the best practices
# have not been followed, so we will fail the job later but print a message here.
check_diff(){
    git diff --exit-code
    code=$?
    if [[ $code -ne 0 ]]; then
        echo "[ERROR] You may need to do some cleanup in the files you commited, see the git diff output above."
    fi
    git checkout -- .
    return_code=$(($return_code + $code))
}

# List the files that are different from the trunk
for f in $(git diff --name-only refs/remotes/origin/trunk | sort -u); do
    if [[ "${f}" =~ navit/support/ ]] || [[ "${f}" =~ navit/fib-1\.1/ ]] || [[ "${f}" =~ navit/traffic/permanentrestrictions/ ]] ; then
        echo "[DEBUG] Skipping file ${f} ..."
        continue
    fi
    if [[ -e "${f}" ]]; then

        # Checks for trailing spaces
        if [[ "${f: -4}" != ".bat" ]]; then
            echo "[INFO] Checking for trailing spaces on ${f}..."
            if [[ "$(file -bi """${f}""")" =~ ^text ]]; then
                sed 's/\s*$//' -i "${f}"
                check_diff
            fi
        fi

        # Formats any *.c and *.cpp files
        if [[ "${f: -2}" == ".c" ]] || [[ "${f: -4}" == ".cpp" ]]; then
            echo "[INFO] Checking for indentation and style compliance on ${f}..."
            astyle --indent=spaces=4 --style=attach -n --max-code-length=120 -xf -xh "${f}"
            check_diff
        fi

        if [[ "${f: -11}" == "shipped.xml" ]]; then
            echo "[INFO] Checking for compliance with the DTD using xmllint on ${f}..."
            xmllint --noout --dtdvalid navit/navit.dtd "$f"
            rc=$?
            if [[ $rc -ne 0 ]]; then
                echo "[ERROR] Your ${f} file doesn't validate against the navit/navit.dtd using xmllint"
            fi
        fi
    fi
done

exit $return_code
