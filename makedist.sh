#!/bin/sh

./prepare.sh

rm -Rf build
mkdir build
cd build
../configure
make dist
