#!/bin/sh

set -e

for i in array bayes genome intruder kmeans labyrinth ssca2 vacation yada; do
  cd $i
  if [ "${ITM}" = "1" ]; then
    make -f Makefile.stm.itm clean
    make -f Makefile.stm.itm
  else
    make -f Makefile.stm clean
    make -f Makefile.stm
  fi
  cd ..
done
