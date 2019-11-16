#!/bin/sh
set -e -u

./generate-decomposition-info.py > ../utfdecode_decompose.cpp
./massage_blocks.py > ../utfdecode_blocks.cpp
./massage_unicode_data.py > ../utfdecode_code_point.cpp
