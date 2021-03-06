#!/bin/bash

set -e

if [ $# -lt 2 ]; then
  echo "$0 <build_type = sim | real> <threads>"
  exit
fi

if [ "$1" == "real" ]; then
  declare -a RUN=("./array/array -t"
                  "./bayes/bayes -v32 -r4096 -n10 -p40 -i2 -e8 -s1 -t"
                  "./genome/genome -g16384 -s64 -n16777216 -t"
                  "./intruder/intruder -a10 -l128 -n262144 -s1 -t"
                  "./kmeans/kmeans -m40 -n40 -t0.00001 -i kmeans/inputs/random-n65536-d32-c16.txt -p"
                  "./kmeans/kmeans -m15 -n15 -t0.00001 -i kmeans/inputs/random-n65536-d32-c16.txt -p"
                  "./labyrinth/labyrinth -i labyrinth/inputs/random-x512-y512-z7-n512.txt -t"
                  "./ssca2/ssca2 -s20 -i1.0 -u1.0 -l3 -p3 -t"
                  "./vacation/vacation -n2 -q90 -u98 -r1048576 -t4194304 -c"
                  "./vacation/vacation -n4 -q60 -u90 -r1048576 -t4194304 -c"
                  "./yada/yada -a15 -i yada/inputs/ttimeu1000000.2 -t")
elif [ "$1" == "sim" ]; then
  declare -a RUN=("./array/array -n 10 -o 100 -t"
                  "./bayes/bayes -v32 -r1024 -n2 -p20 -s0 -i2 -e2 -t"
                  "./genome/genome -g256 -s16 -n16384 -t"
                  "./intruder/intruder -a10 -l4 -n2048 -s1 -t"
                  "./kmeans/kmeans -m40 -n40 -t0.05 -i kmeans/inputs/random-n2048-d16-c16.txt -p"
                  "./kmeans/kmeans -m15 -n15 -t0.05 -i kmeans/inputs/random-n2048-d16-c16.txt -p"
                  "./labyrinth/labyrinth -i labyrinth/inputs/random-x32-y32-z3-n96.txt -t"
                  "./ssca2/ssca2 -s13 -i1.0 -u1.0 -l3 -p3 -t"
                  "./vacation/vacation -n2 -q90 -u98 -r16384 -t4096 -c"
                  "./vacation/vacation -n4 -q60 -u90 -r16384 -t4096 -c"
                  "./yada/yada -a20 -i yada/inputs/633.2 -t")
else
  echo "Error: build_type"
  exit
fi

for i in "${RUN[@]}"; do
  declare -a CMD=($i$2)
  echo "Executing: ${CMD[@]}"
  FILENAME="$(basename ${CMD[0]})"

  while [ -f "${FILENAME}.log" ]; do
    FILENAME+="0"
  done
  FILENAME+=".log"

  hostname > "${FILENAME}"
  NO_AVX2=1 STM_STATS=1 timeout --foreground --preserve-status 30m ${CMD[@]} >> "${FILENAME}" 2>&1
done
