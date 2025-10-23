#!/bin/bash
set -e

# in case build is executed from outside current dir be a gem and change the dir
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd -P)"
cd $CURRENT_DIR

if [ ! -f /.dockerenv ]; then
  echo "ERROR: This script is only supported running in docker"
  exit 1
fi

export BUILD_DIR="$PWD"
export BUILD_PREFIX="$BUILD_DIR/build"
export TOOLCHAIN_DIR="/opt/toolchains/mips-gcc720-glibc229/bin"
export GCC_PREFIX=mips-linux-gnu
export TOOLCHAIN=${TOOLCHAIN_DIR}/${GCC_PREFIX}
export CFLAGS="-I${BUILD_PREFIX}/include/"
export LDFLAGS="-L${BUILD_PREFIX}/lib/"
export CC="${TOOLCHAIN}-gcc"
export AR="${TOOLCHAIN}-ar"
export LD="${TOOLCHAIN}-ld"

export PATH="$TOOLCHAIN_DIR:$PATH"

cd $BUILD_DIR

if [ -d build ]; then
    echo "Cleaning up old builds..."
    rm -rf build/*
else
    echo "Creating ./build for prefix target..."
    mkdir -p build
fi

build_jpeg9() {
  cd $CURRENT_DIR/jpeg-9d
  ./configure --host=x86_64 --build=mips --prefix=$BUILD_PREFIX
  make
  make install
}

build_libsynchronous() {
  cd $CURRENT_DIR/libsynchronous-frames

  mkdir -p _build && cd _build
  cmake ..
  make
  cmake --install . --prefix $BUILD_PREFIX
}

build_mjpg_streamer() {
  cd $CURRENT_DIR/mjpg-streamer/mjpg-streamer-experimental

  make

  cd _build
  cmake --install . --prefix $BUILD_PREFIX
}

# Fixme - convert to Makefile
build_jpeg9
build_libsynchronous
build_mjpg_streamer

cd $CURRENT_DIR/build/
tar -zcvf ../mjpg-streamer.tar.gz *
