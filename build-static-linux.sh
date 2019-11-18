#!/bin/sh

./prepare.sh

export WORKDIR=/opt/src

COMMANDS="apk update"
COMMANDS="$COMMANDS && apk add --no-cache binutils make libgcc musl-dev g++ autoconf automake"
COMMANDS="$COMMANDS && cd ${WORKDIR}"
COMMANDS="$COMMANDS && ./prepare.sh"
COMMANDS="$COMMANDS && mkdir build-musl"
COMMANDS="$COMMANDS && cd build-musl"
COMMANDS="$COMMANDS && pwd && ls -lha && ls -lha .."
COMMANDS="$COMMANDS && CXX=\"musl-gcc -static\" ../configure"
COMMANDS="$COMMANDS && make"

rm -rf build-musl

docker run --rm -it -v "${PWD}":"${WORKDIR}" alpine:latest sh -c "$COMMANDS"


