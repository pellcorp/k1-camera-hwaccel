#!/bin/bash

export BUILD_DIR="$PWD"
export SOURCE_DIR="$BUILD_DIR/packages"
export BUILD_PREFIX="$BUILD_DIR/local"

# pellcorp/k1-klipper-fw-build will have /opt/toolchains/mips-gcc720-glibc229/
if [ -d /opt/toolchains/mips-gcc720-glibc229/ ]; then
  export TOOLCHAIN_DIR="/opt/toolchains/mips-gcc720-glibc229/bin"
else
  export TOOLCHAIN_DIR="$BUILD_DIR/toolchains/mips-gcc720-glibc229/bin"
fi

export GCC_PREFIX=mips-linux-gnu
export TOOLCHAIN=${TOOLCHAIN_DIR}/${GCC_PREFIX}
export CFLAGS="-I${BUILD_PREFIX}/include/"
export LDFLAGS="-L${BUILD_PREFIX}/lib/"
export CC="${TOOLCHAIN}-gcc"
export AR="${TOOLCHAIN}-ar"
export LD="${TOOLCHAIN}-ld"

export PATH="$TOOLCHAIN_DIR:$PATH"
