#!/bin/bash
set -e

wget https://packages.cloud.google.com/apt/doc/apt-key.gpg && sudo apt-key add apt-key.gpg

#sudo apt-get update
sudo apt-get install -y cmake gettext libsaxonb-java librsvg2-bin pkg-config rename
gem install --no-document git


