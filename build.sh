#!/bin/bash
set -e

# in case build is executed from outside current dir be a gem and change the dir
BUILD_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd -P)"
cd $BUILD_DIR

if [ ! -f /.dockerenv ]; then
  echo "ERROR: This script is only supported running in docker"
  exit 1
fi

export BUILD_PREFIX="$BUILD_DIR/build/mjpg-streamer"
# this is the docker toolchain
export TOOLCHAIN_DIR="/opt/toolchains/mips-gcc720-glibc229/bin"
export GCC_PREFIX=mips-linux-gnu
export TOOLCHAIN=${TOOLCHAIN_DIR}/${GCC_PREFIX}
export CFLAGS="-I${BUILD_PREFIX}/include/"
export LDFLAGS="-L${BUILD_PREFIX}/lib/"
export CC="${TOOLCHAIN}-gcc"
export AR="${TOOLCHAIN}-ar"
export LD="${TOOLCHAIN}-ld"

export PATH="$TOOLCHAIN_DIR:$PATH"

if [ -d build ]; then
    rm -rf build/
fi
mkdir -p build/mjpg-streamer

# Fixme - convert to Makefile
build_jpeg9() {
  cd $BUILD_DIR/jpeg-9d
  ./configure --host=x86_64 --build=mips --prefix=$BUILD_PREFIX
  make
  make install
}

build_libsynchronous() {
  cd $BUILD_DIR/libsynchronous-frames
  [ -d _build ] && rm -rf _build
  mkdir -p _build
  cd _build
  cmake ..
  make
  cmake --install . --prefix $BUILD_PREFIX
}

build_mjpg_streamer() {
  cd $BUILD_DIR/mjpg-streamer/mjpg-streamer-experimental
  [ -d _build ] && rm -rf _build
  make
  cd _build
  cmake --install . --prefix $BUILD_PREFIX
}

build_jpeg9
build_libsynchronous
build_mjpg_streamer

cd $BUILD_DIR/build/
tar -zcvf mjpg-streamer.tar.gz mjpg-streamer
