#!/bin/sh
set -e

message=$(git log -1 --pretty=%B)
git config --global user.name "CircleCI"
git config --global user.email circleci@navit-project.org
git rebase trunk
git checkout trunk
git pull
git merge --squash ${CIRCLE_BRANCH}
git commit -m "${message}"
git push
