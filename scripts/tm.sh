#!/bin/sh

ln -sf ../$1/Defines.common.mk common/Defines.common.mk
ln -sf ../$1/Makefile.stm common/Makefile.stm
ln -sf ../$1/tm.h lib/tm.h
