#!/usr/bin/env bash
set -u

# #####################################################################################################################
# This script runs on CI to verify that the code of the PR has been sanitized before push.
# It has one Optional Parameter. If a filename is passed as the first parameter a "git diff" will be generated into
# the path given. This can be used to provide the user with the diff from the on CI run of this script which the User
# can then import and commit.
#
# WARNING: make sure you commit ALL your changes before running it locally if you ever do it because it will run a git
# checkout -- which will reset your changes on all files...
# #####################################################################################################################

return_code=0
GIT_DIFF_OUTPUT="${1:-/dev/stdout}"
KEEPTEMP=${2:-false}

cleanup() {
	$KEEPTEMP || rm -rf build-locale
}
trap cleanup EXIT


# Check if any file has been modified. If yes, that means the best practices
# have not been followed, so we will fail the job later but print a message here.
check_diff(){
    git --no-pager diff > "${GIT_DIFF_OUTPUT}"
    git --no-pager diff --exit-code --stat
    code=$?
    if [[ $code -ne 0 ]]; then
        echo "[ERROR] You may need to do some cleanup in the file $*, see the git diff output above." >&2
    fi
    return_code=$(($return_code + $code))
}

check_po() {
	local retval=0

	mkdir -p build-locale
	cd build-locale
	cmake .. >/dev/null
	cd po
	make -j5 locales >/dev/null

	for po in *.po
	do
		if diff -u $po ../../po/$po.in 2>&1 | grep -Eq "[+-]msgid"
		then
			echo "[ERROR] new or changed msgid found in $po.in" >&2
			cp $po ../../po/$po.in
			retval=2
		fi
	done
	cd ../..
	return $retval
}

check_po || {
	echo "[ERROR] found outdated po-files. Please check the differences in the current source tree for po/*.po" >&2
	exit 2
}

git config --global --add safe.directory $(pwd)

# List the files that are different from the trunk
for f in $(git --no-pager diff --name-only refs/remotes/origin/trunk | sort -u); do
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
            fi
        fi

        # Formats any *.h *.c and *.cpp files
        if [[ "${f: -2}" == ".h" ]] || [[ "${f: -2}" == ".c" ]] || [[ "${f: -4}" == ".cpp" ]]; then
            echo "[INFO] Checking for indentation and style compliance on ${f}..."
            clang-format -i "${f}"
        fi

       fi
done


echo "[INFO] Checking for compliance with the DTD using xmllint on ${f}..."
if ! find . -type f -name "*shipped.xml" -print0 | xargs -0L1 xmllint --noout --dtdvalid navit/navit.dtd; then
		echo "[ERROR] one of the \*shipped.xml-files doesn't validate against the navit/navit.dtd using xmllint" >&2
		return_code=3
fi

check_diff
exit $return_code
