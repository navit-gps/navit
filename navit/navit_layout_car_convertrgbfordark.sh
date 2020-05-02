#!/bin/bash
echo Light: $1
echo Dark:\  $(printf '%02x' $(echo $(printf "%d" 0x${1:0:2})/34 | bc))$(printf '%02x' $(echo $(printf "%d" 0x${1:2:2})/8 | bc))$(printf '%02x' $(echo $(printf "%d" 0x${1:4:2})/9 | bc))
