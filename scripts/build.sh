#!/bin/sh

set -e

for i in bayes genome intruder kmeans labyrinth ssca2 vacation yada; do
  cd $i
  make -f Makefile.stm clean
  make -f Makefile.stm
  cd ..
done
