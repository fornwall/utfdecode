AM_CXXFLAGS = -Wall -Wextra -std=c++14 -pedantic

bin_PROGRAMS = utfdecode

utfdecode_SOURCES = utfdecode.cpp \
					utfdecode_utf16.cpp \
					utfdecode.hpp \
					utfdecode_decompose.cpp \
					utfdecode_blocks.cpp \
					utfdecode_category.cpp \
					utfdecode_code_point.cpp \
					musl/wcwidth_musl.h \
					musl/wide.h \
					musl/wcwidth.cpp \
					musl/nonspacing.h

dist_man1_MANS = utfdecode.1
