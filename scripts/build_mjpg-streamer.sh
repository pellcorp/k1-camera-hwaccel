#!/bin/bash
cd $SOURCE_DIR

if [ ! -d "mjpg-streamer" ]; then
    wget "https://git.zevaryx.com/zevaryx/mjpg-streamer/archive/main.tar.gz" -O "mjpg-streamer.tar.gz"
    tar -zxf "mjpg-streamer.tar.gz"
    rm "mjpg-streamer.tar.gz"
fi

#echo "Applying patch for libjpeg"
#cd $BUILD_DIR
#patch -p1 < 000-remove-jpeglib.patch
#cd $SOURCE_DIR

cd "mjpg-streamer/mjpg-streamer-experimental"

make

cd _build
cmake --install . --prefix $BUILD_PREFIX
