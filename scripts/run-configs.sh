#!/bin/sh

set -e

if [ $# -ne 2 ]; then
  echo "$0 <outdir> <iterations>"
  exit
fi

# mutex
ORIGINAL=1 ./scripts/build.seq.sh && ./scripts/run-threads.sh ./scripts/abort.sh ${2} ${1}/sequential
# tardistm, no repair
cd ../tardisTM && ln -sf Makefile.restart Makefile && make clean && make && cd ../stamp && ORIGINAL=1 ./scripts/build.stm.sh && ./scripts/run-threads.sh ./scripts/abort.sh ${2} ${1}/tardistm-none
# tardistm, repair
cd ../tardisTM && ln -sf Makefile.merge Makefile && make clean && make && cd ../stamp && ./scripts/build.stm.sh && ./scripts/run-threads.sh ./scripts/abort.sh ${2} ${1}/tardistm

# gcc itm
cd ../tardisTM && ln -sf Makefile.restart Makefile && make clean && cd ../stamp && ITM=1 ./scripts/build.stm.sh && ./scripts/run-threads.sh ./scripts/abort.sh ${2} ${1}/gcc
# gcc itm with intel rtx
cd ../tardisTM && ln -sf Makefile.restart Makefile && make clean && cd ../stamp && ITM=1 ./scripts/build.stm.sh && ./scripts/run-threads.sh ./scripts/abort.htm.sh ${2} ${1}/gcc-rtm
