#!/bin/bash
cd $SOURCE_DIR

cd "mjpg-streamer/mjpg-streamer-experimental"

make

cd _build
cmake --install . --prefix $BUILD_PREFIX
