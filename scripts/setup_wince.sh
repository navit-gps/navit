#!/bin/bash

# #############################################################
# This script has been deprecated. If you need to add packages,
# add it to the wince/Dockerfile on the navit/Dockerfiles repo.
# #############################################################

set -e

mkdir -p /var/lib/apt/lists/partial
apt-get update
apt-get install -y git-core
apt-get install -y xsltproc
