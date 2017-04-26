#!/bin/sh
espeak -s150 -a 200 -vde "$1" --stdout | paplay
