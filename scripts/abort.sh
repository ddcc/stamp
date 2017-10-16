#!/bin/bash

set -e

if [ $# -lt 2 ]; then
  echo "$0 <iterations> <threads>"
  exit
fi

COUNTER=0
while [ $COUNTER -lt $1 ]; do
  LOG="abort${COUNTER}.csv"
  echo "${LOG}"

  rm -f *.log

  $(dirname $0)/run.sh real $2

  INNER=0
  for i in *.log; do
      FILENAME=$(basename $i)
      FILENAME=${FILENAME%.*}

      if [ $INNER -eq 0 ]; then
        echo -e "Benchmark\nTime" > "${LOG}"
        grep -E "[A-Za-z() -]+: [0-9]+" ${i} | cut -d : -f 1 >> "${LOG}"
      fi

      { echo "${FILENAME}"; grep -E "[tT]ime[a-z:= ]* |[A-Za-z() -]+: [0-9]+" ${i}; } | sed "s/[A-Za-z() -]*[:|=][ ]*//" | paste -d, "${LOG}" - > "${LOG}.tmp"
      mv "${LOG}.tmp" "${LOG}"

      INNER=$((INNER+1))
  done

  COUNTER=$((COUNTER+1))
done
