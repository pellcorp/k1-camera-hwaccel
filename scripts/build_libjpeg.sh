#!/bin/bash
cd $SOURCE_DIR

cd "jpeg-9d"

./configure --host=x86_64 --build=mips --prefix=$BUILD_PREFIX
make
make install