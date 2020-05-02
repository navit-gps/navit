#!/bin/bash
echo Light: $1
r=$(printf '%02x' $(echo $(printf "%d" 0x${1:0:2})/34 | bc))
g=$(printf '%02x' $(echo $(printf "%d" 0x${1:2:2})/8 | bc))
b=$(printf '%02x' $(echo $(printf "%d" 0x${1:4:2})/9 | bc))
echo Dark:\  $r$g$b
