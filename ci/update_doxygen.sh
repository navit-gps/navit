#!/bin/bash
set -e
mkdir -p ~/.ssh/
ssh-keyscan github.com >> ~/.ssh/known_hosts
git clone -b gh-pages git@github.com:navit-gps/navit.git /root/navit-doc
cd /root/navit-doc
git config --global push.default simple
git config user.name "Circle CI"
git config user.email "circleci@navit-project.org"
rsync -vrtza --exclude '.git' --delete /root/project/doc/html/ /root/navit-doc/
echo "" > .nojekyll
echo "doxygen.navit-project.org" > CNAME
git add .
git commit -am "update:doc:Doxygen update for commit ${CIRCLE_SHA1} [ci skip]" || true
git push
