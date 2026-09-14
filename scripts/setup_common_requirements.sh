#!/bin/sh
set -e

apt-get update && apt-get install -y --no-install-recommends wget unzip cmake build-essential \
  gettext librsvg2-bin util-linux git ssh sed astyle \
  libosmium2-dev libprotozero-dev libexpat1-dev zlib1g-dev libbz2-dev
