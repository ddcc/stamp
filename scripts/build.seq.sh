#!/bin/sh

set -e

for i in array bayes genome intruder kmeans labyrinth ssca2 vacation yada; do
  cd $i
  make -f Makefile.seq clean
  make -f Makefile.seq
  cd ..
done
