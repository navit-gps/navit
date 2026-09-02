#!/usr/bin/env bash
set -u

# #####################################################################################################################
# This script runs on CI to verify that the code of the PR has been sanitized before push.
# It has one Optional Parameter. If a filename is passed as the first parameter a "git diff" will be generated into
# the path given. This can be used to provide the user with the diff from the on CI run of this script which the User
# can then import and commit.
#
# WARNING: This script edits files in the working tree (trailing spaces, clang-format).
# Commit or stash first if you need a clean snapshot. Set SANITY_GIT_BASE if your clone
# has no origin/trunk (defaults try main/master, then @{upstream}, then HEAD).
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
	make -j5 locales >/dev/null || return 1

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
	echo "[ERROR] found incorrect or outdated po-files. Please check the differences in the current source tree for po/*.po" >&2
	exit 2
}

git config --global --add safe.directory $(pwd)

# Files to sanitize: working tree vs a merge target. Navit CI uses origin/trunk; many
# clones only have a feature remote. Override with SANITY_GIT_BASE if needed.
SANITY_GIT_BASE="${SANITY_GIT_BASE:-}"
if [[ -z "${SANITY_GIT_BASE}" ]]; then
	if git rev-parse -q --verify refs/remotes/origin/trunk >/dev/null 2>&1; then
		SANITY_GIT_BASE="refs/remotes/origin/trunk"
	elif git rev-parse -q --verify refs/remotes/origin/main >/dev/null 2>&1; then
		SANITY_GIT_BASE="refs/remotes/origin/main"
	elif git rev-parse -q --verify refs/remotes/origin/master >/dev/null 2>&1; then
		SANITY_GIT_BASE="refs/remotes/origin/master"
	elif git rev-parse -q --verify '@{upstream}' >/dev/null 2>&1; then
		SANITY_GIT_BASE="@{upstream}"
	else
		echo "[WARN] No origin/trunk, origin/main, origin/master, or @{upstream}; set SANITY_GIT_BASE." >&2
		echo "[WARN] Falling back to HEAD (only sees uncommitted changes vs last commit)." >&2
		SANITY_GIT_BASE="HEAD"
	fi
fi
echo "[INFO] Sanitize: git diff --name-only ${SANITY_GIT_BASE} (working tree vs base)"

# List the files that are different from the base ref
for f in $(git --no-pager diff --name-only "${SANITY_GIT_BASE}" | sort -u); do
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


echo "[INFO] Checking for compliance with the DTD using xmllint on *shipped.xml"
if ! find . -type f -name "*shipped.xml" -print0 | xargs -0L1 xmllint --noout --dtdvalid navit/navit.dtd; then
		echo "[ERROR] one of the \*shipped.xml-files doesn't validate against the navit/navit.dtd using xmllint" >&2
		return_code=3
fi

check_diff
exit $return_code
