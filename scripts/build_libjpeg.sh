#!/bin/bash
cd $SOURCE_DIR

if [ ! -d "jpeg-9d" ]; then
    wget "https://www.ijg.org/files/jpegsrc.v9d.tar.gz"
    tar -zxf "jpegsrc.v9d.tar.gz"
    rm "jpegsrc.v9d.tar.gz"
fi

cd "jpeg-9d"

./configure --prefix=$BUILD_PREFIX
make
make install