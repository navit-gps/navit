#!/bin/sh
set -e

apt-get update && apt-get install -y wget unzip cmake build-essential gettext librsvg2-bin util-linux git ssh sed astyle
