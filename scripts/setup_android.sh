#!/bin/bash
set -e

wget https://packages.cloud.google.com/apt/doc/apt-key.gpg && sudo apt-key add apt-key.gpg

sudo apt-get update
sudo apt-get install -y cmake gettext libsaxonb-java librsvg2-bin pkg-config rename
gem install --no-document domain_name -v 0.5.20190701
gem install --no-document rchardet -v 1.8.0
gem install --no-document public_suffix -v 5.1.1
gem install --no-document git -v 1.19.1
gem install --no-document faraday -v 2.8.1
gem install --no-document faraday-net_http -v 3.0.2
gem install --no-document signet -v 0.18.0
gem install --no-document google-cloud-errors -v 1.3.1
gem install --no-document google-cloud-env -v 1.6.0
gem install --no-document google-cloud-core -v 1.6.1
gem install --no-document drb -v 2.0.6
gem install --no-document activesupport -v 6.1.7.8
gem install --no-document google-cloud-storage -v 1.31.1
gem install --no-document excon -v 0.109.0
gem install --no-document fastlane


