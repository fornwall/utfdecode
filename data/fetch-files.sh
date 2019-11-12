#!/bin/sh
set -e -u

UNICODE_VERSION=12.1.0

curl -o Blocks.txt https://www.unicode.org/Public/$UNICODE_VERSION/ucd/Blocks.txt
curl -o UnicodeData.txt https://www.unicode.org/Public/$UNICODE_VERSION/ucd/UnicodeData.txt
