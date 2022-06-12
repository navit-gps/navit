#!/bin/bash
set -e

sudo apt-get update
sudo apt-get install -y cmake gettext libsaxonb-java librsvg2-bin pkg-config rename ruby ruby-dev build-essential rake-compiler git default-jdk-headless
gem install rake
gem install --no-document fastlane git

