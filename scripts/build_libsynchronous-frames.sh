#!/bin/bash
cd $SOURCE_DIR

if [ ! -d "libsynchronous-frames" ]; then
    wget "https://git.zevaryx.com/zevaryx/libsynchronous-frames/archive/main.tar.gz" -O "libsynchronous-frames.tar.gz"
    tar -zxf "libsynchronous-frames.tar.gz"
    rm "libsynchronous-frames.tar.gz"
fi

cd "libsynchronous-frames/"

mkdir -p _build && cd _build
cmake ..
make
cmake --install . --prefix $BUILD_PREFIX
