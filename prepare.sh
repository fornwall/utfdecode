#!/bin/sh

aclocal
automake --add-missing
autoreconf

./configure --prefix=$HOME/lib/prefix
make
make install
