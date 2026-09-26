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

changed="$(git diff --name-only "$mb..HEAD" -- '*.rst')"
if [ -z "$changed" ]; then
    echo "No .rst files changed since $mb ($base), nothing to check."
    exit 0
fi

sphinx-build -q --keep-going -b html docs "$outdir" 2> "$log"
code=$?
if [ $code -ne 0 ]; then
    echo "sphinx-build failed with exit code $code." >&2
    cat "$log" >&2
    exit $code
fi

if [ ! -s "$log" ]; then
    echo "Docs build produced no warnings."
    exit 0
fi

hits=0
while IFS= read -r file; do
    if grep -F -q "$file" "$log"; then
        printf '::error file=%s::Sphinx warning in a documentation file changed by this branch\n' "$file"
        grep -F "$file" "$log"
        hits=1
    fi
done <<< "$changed"

if [ "$hits" -eq 0 ]; then
    echo "Sphinx warnings are limited to files not changed by this branch."
    exit 0
fi

exit 1
