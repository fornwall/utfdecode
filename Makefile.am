AM_CXXFLAGS = -Wall -Wextra -std=c++1y -pedantic -Os

bin_PROGRAMS = utfdecode

utfdecode_SOURCES = utfdecode.cpp musl/wcwidth_musl.h musl/wide.h musl/wcwidth.cpp musl/nonspacing.h

dist_man1_MANS = utfdecode.1

# Installs the files in $PREFIX/share/$pkgname/
dist_pkgdata_DATA = data/UnicodeData.txt data/Blocks.txt
AM_CPPFLAGS = -DDATADIR=\"$(pkgdatadir)\"
