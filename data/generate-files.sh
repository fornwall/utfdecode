#!/bin/sh
set -e -u

./massage_blocks.py > ../unicode_blocks.hpp
./massage_unicode_data.py > ../unicode_code_points.hpp
