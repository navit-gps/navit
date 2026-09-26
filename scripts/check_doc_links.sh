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
#   DOCS_BASE  base ref to diff against (default: origin/trunk)

set -uo pipefail

base="${DOCS_BASE:-origin/trunk}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="$(mktemp -d)"
outdir="$build_root/html"
log="$build_root/warnings.log"

cd "$root"

changed="$(git diff --name-only "$base"...HEAD -- '*.rst')"
if [ $? -ne 0 ]; then
    echo "Unable to determine changed files against $base." >&2
    exit 1
fi
if [ -z "$changed" ]; then
    echo "No .rst files changed since $base, nothing to check."
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
