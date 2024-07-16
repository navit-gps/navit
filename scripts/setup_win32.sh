#!/bin/bash
set -e

apt-get update && xargs -a scripts/setup_24.04_requirements.list apt-get install -y