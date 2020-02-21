#!/bin/bash

set -e

if [ $# -ne 3 ]; then
  echo "$0 <script> <iterations> <directory>"
  exit
fi

mkdir -p "${3}"
cp ../tinySTM/Makefile "${3}"

for i in "1" "2" "4" "8" "16" "32" "64"; do
  DIR="${3}/${i}"
  mkdir -p "${DIR}"
  ${1} ${2} ${i}
  mv *.csv temp/* "${DIR}"
done

$(dirname ${0})/combine.py "${3}"
$(dirname ${0})/speedup.py "${3}/../sequential" "${3}"
