#!/usr/bin/env bash
# Shared resolution of the CI base commit. Sourced by CI scripts, so it avoids
# set -e/-u assumptions of its own.

CI_NULL_SHA=0000000000000000000000000000000000000000
CI_UPSTREAM_REPO=${CI_UPSTREAM_REPO:-navit-gps/navit}

resolve_mergebase() {
    local base=
    local remote=
    local branch=
    local url=

    if [ -n "${BASE_COMMIT:-}" ] && [ "$BASE_COMMIT" != "$CI_NULL_SHA" ]; then
        base=$BASE_COMMIT
    elif [ -n "${TARGET_BRANCH:-}" ] && git cat-file -e "${TARGET_BRANCH}^{commit}" 2>/dev/null; then
        base=$TARGET_BRANCH
    elif git show-ref --verify --quiet refs/remotes/origin/HEAD; then
        base=refs/remotes/origin/HEAD
    else
        for remote in $(git remote); do
            url=$(git remote get-url "$remote" 2>/dev/null) || continue
            case "$url" in
            *"$CI_UPSTREAM_REPO"*)
                if git show-ref --verify --quiet "refs/remotes/$remote/trunk"; then
                    base="refs/remotes/$remote/trunk"
                    break
                fi
                ;;
            esac
        done
        if [ -z "$base" ] && git show-ref --verify --quiet refs/remotes/origin/trunk; then
            base=refs/remotes/origin/trunk
        fi
    fi

    if [ -z "$base" ]; then
        return 1
    fi

    if ! git cat-file -e "$base^{commit}" 2>/dev/null; then
        case "$base" in
        refs/remotes/*/*)
            remote=${base#refs/remotes/}
            remote=${remote%%/*}
            branch=${base#refs/remotes/$remote/}
            git fetch --no-tags "$remote" "$branch" >/dev/null 2>&1 || true
            ;;
        *)
            git fetch --no-tags origin "$base" >/dev/null 2>&1 || true
            ;;
        esac
    fi

    git merge-base HEAD "$base" 2>/dev/null || return 1
}
