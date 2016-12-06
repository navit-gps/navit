git rebase master
git checkout master
git merge --squash ${CIRCLE_BRANCH}
git commit
