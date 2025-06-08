#!/bin/bash
set -e

wget https://packages.cloud.google.com/apt/doc/apt-key.gpg && sudo apt-key add apt-key.gpg

sudo apt-get update
sudo apt-get install -y cmake gettext libsaxonb-java librsvg2-bin pkg-config rename

# The docker image's ruby version is really old now, so we need to reinstall a more recent ruby
sudo apt-get install -y git curl libssl-dev libreadline-dev zlib1g-dev \
    autoconf bison build-essential libyaml-dev \
    libreadline-dev libncurses5-dev libffi-dev libgdbm-dev
curl -sL https://raw.githubusercontent.com/rbenv/rbenv-installer/refs/heads/main/bin/rbenv-installer | bash -
echo 'export PATH="$HOME/.rbenv/bin:$PATH"' >> ~/.bashrc
echo 'eval "$(rbenv init -)"' >> ~/.bashrc
source ~/.bashrc
echo PATH:"$PATH"
echo Shell:$0
rbenv -v
rbenv install 2.7.0
rbenv global 2.7.0

# What we actually want is fastlane but because out ruby version is not that recent (2.7.0), we need to manually pin dependencies to compatible versions before installing fastlane
gem install --no-document public_suffix -v 5.1.1
gem install --no-document signet -v 0.19.0
gem install --no-document google-cloud-errors -v 1.4.0
gem install --no-document google-cloud-core -v 1.7.1
gem install --no-document fastlane
