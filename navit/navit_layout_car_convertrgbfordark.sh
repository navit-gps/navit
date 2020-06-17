#!/bin/bash
if [[ $1 =~ ^[0-9a-fA-F]{6}$ ]]; then
  echo Light: $1
  echo Dark:\  $(printf '%02x' $(echo $(printf "%d" 0x${1:0:2})/34 | bc))$(printf '%02x' $(echo $(printf "%d" 0x${1:2:2})/8 | bc))$(printf '%02x' $(echo $(printf "%d" 0x${1:4:2})/9 | bc))
else
  echo Usage: $0 RRGGBB
fi
