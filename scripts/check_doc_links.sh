#!/usr/bin/env bash
# Fail the docs build if Sphinx reports a problem in any .rst file that was
# touched by the current branch.
#
# Sphinx has to build the whole documentation set to resolve cross references,
# but legacy pages (the "Old Wiki" toctree and pages not yet folded into the
# new structure) still contain warnings. Tolerate those; only fail on warnings
# attributed to files the current branch modified. Once a file is touched it
# must already be clean.
#
# Usage: bash scripts/check_doc_links.sh
#   DOCS_BASE   base ref or SHA to diff against; for a pull request this is
#               the base branch SHA, for a push it is the "before" SHA.
#   DOCS_EVENT  path to the GitHub event payload, used to recover the base
#               when DOCS_BASE is empty.

set -uo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="$(mktemp -d)"
trap 'rm -rf "$build_root"' EXIT
outdir="$build_root/html"
log="$build_root/warnings.log"

cd "$root"

# Resolve the base to diff against. The only refs that describe the intended
# merge point are the pull request's base SHA and the push event's "before"
# SHA, so take one of those and refuse to guess. Falling back to a moving
# branch like origin/trunk would quietly judge the change against a different
# set of files whenever the two disagree, which is worse than failing.
base="${DOCS_BASE:-}"
if [ -z "$base" ] && [ -n "${DOCS_EVENT:-}" ] && [ -f "$DOCS_EVENT" ]; then
    base="$(jq -r '.pull_request.base.sha // .before // empty' "$DOCS_EVENT")"
fi
if [ -z "$base" ]; then
    echo "No base to diff against: set DOCS_BASE, or point DOCS_EVENT at the event payload." >&2
    exit 1
fi

# A three-dot diff is really a merge-base diff, but git still needs the base
# to be a commit it can see. Resolve the merge point explicitly so a base
# that has moved (or a force-push whose "before" is gone) is reported as
# such instead of being papered over.
if ! mb="$(git merge-base "$base" HEAD 2>/dev/null)" || [ -z "$mb" ]; then
    echo "'$base' is not usable as a diff base in this checkout." >&2
    echo "Fetch it first, or set DOCS_BASE to a ref that is present." >&2
    exit 1
fi

# NUL-separated so that paths containing spaces or quotes survive, and with
# quoting disabled so the list is not mangled by git's C-style escaping.
changed=()
while IFS= read -r -d '' file; do
    changed+=("$file")
done < <(git -c core.quotePath=false diff --name-only -z "$mb..HEAD" -- '*.rst')

# Build unconditionally. Sphinx has to see the whole tree to resolve
# references, and a branch that touches only docs/conf.py, the requirements
# or an image can still break every page. Returning early when no .rst
# changed reported those as a pass, which is exactly the case the gate
# exists to catch.
sphinx-build -q --keep-going -b html docs "$outdir" 2> "$log"
code=$?
if [ $code -ne 0 ]; then
    echo "sphinx-build failed with exit code $code." >&2
    cat "$log" >&2
    exit $code
fi

# Only the warning check is scoped to the branch: trunk still carries
# warnings in pages nobody has cleaned up, and demanding zero of them would
# mean every documentation change had to fix the whole set first.
if [ "${#changed[@]}" -eq 0 ]; then
    echo "No .rst files changed since $mb ($base); the documentation set still builds."
    exit 0
fi

if [ ! -s "$log" ]; then
    echo "Docs build produced no warnings."
    exit 0
fi

# Map every warning onto the file Sphinx blamed, normalised to a path
# relative to the docs source directory so it can be compared against git's
# list. Sphinx reports absolute paths for documents and source-relative ones
# for included files such as CHANGELOG.md, and it omits the line number for
# toctree warnings. Comparing the raw text instead let a touched page
# inherit the warnings of any page whose path merely contained its own.
blamed="$build_root/blamed"
awk -v root="$root/" '
    /: WARNING:/ {
        path = $0
        sub(/:[0-9]+: WARNING:.*$/, "", path)
        sub(/: WARNING:.*$/, "", path)
        sub("^" root "docs/", "", path)
        sub("^" root, "", path)
        sub("^docs/", "", path)
        print path "\t" $0
    }
' "$log" | LC_ALL=C sort -u > "$blamed"

hits=0
for file in "${changed[@]}"; do
    rel="${file#docs/}"
    [ -n "$rel" ] || continue
    # Compare the whole first field, never a substring of it: a bare
    # index.rst is a different page from user/faq/index.rst, and testing
    # for the shorter name inside the longer one blames the wrong page.
    if ! awk -F'\t' -v want="$rel" '$1 == want { found = 1 } END { exit !found }' "$blamed"; then
        continue
    fi
    printf '::error file=%s::Sphinx reported a warning in a page this branch changed\n' "$file"
    awk -F'\t' -v want="$rel" '$1 == want' "$blamed" | cut -f2-
    hits=1
done

if [ "$hits" -eq 0 ]; then
    echo "Sphinx warnings are limited to files not changed by this branch."
    exit 0
fi

exit 1
