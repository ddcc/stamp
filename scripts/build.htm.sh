#!/bin/sh

set -e

for i in array bayes genome intruder kmeans labyrinth ssca2 vacation yada; do
  cd $i
  make -f Makefile.htm clean
  make -f Makefile.htm
  cd ..
done
