#!/bin/bash
cd $SOURCE_DIR

cd "libsynchronous-frames/"

mkdir -p _build && cd _build
cmake ..
make
cmake --install . --prefix $BUILD_PREFIX
