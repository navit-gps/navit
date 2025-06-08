#!/bin/bash
set -e

ruby --version

# What we actually want is fastlane but because out ruby version is not that recent (2.7.0), we need to manually pin dependencies to compatible versions before installing fastlane
gem install --no-document public_suffix -v 5.1.1
gem install --no-document signet -v 0.19.0
gem install --no-document google-cloud-errors -v 1.4.0
gem install --no-document google-cloud-core -v 1.7.1
gem install --no-document fastlane
