#!/bin/sh

./prepare.sh

rm -Rf build
mkdir build
cd build
../configure --prefix=$HOME/lib/prefix
make dist
make install
