#!/bin/bash
set -e

sudo apt-get update
sudo apt-get install -y cmake gettext libsaxonb-java librsvg2-bin pkg-config rename
gem install --no-document fastlane git

