#!/bin/sh
set -e -u

UNICODE_VERSION=12.1.0
URL_PREFIX=https://www.unicode.org/Public/$UNICODE_VERSION/ucd

curl -O $URL_PREFIX/Blocks.txt
curl -O $URL_PREFIX/CompositionExclusions.txt
curl -O $URL_PREFIX/extracted/DerivedCombiningClass.txt
curl -O $URL_PREFIX/UnicodeData.txt
