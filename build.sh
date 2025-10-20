#!/bin/bash
set -e

source env.sh

echo $CFLAGS
if ! command -v cmake >/dev/null 2>&1; then
    echo "Please install cmake first"
elif ! command -v wget >/dev/null 2>&1; then
    echo "Please install wget first"
elif ! command -v make >/dev/null 2>&1; then
    echo "Please install make first"
fi

cd $BUILD_DIR

if [ ! -d toolchains/mips-gcc720-glibc229 ]; then
    echo "Getting toolchain..."
    mkdir -p toolchains && cd toolchains
    wget "https://github.com/ballaswag/k1-discovery/releases/download/1.0.0/mips-gcc720-glibc229.tar.gz"
    tar -xf mips-gcc720-glibc229.tar.gz
    rm mips-gcc720-glibc229.tar.gz
    cd $BUILD_DIR
fi

if [ -d local ]; then
    echo "Cleaning up old builds..."
    rm -rf local/*
else
    echo "Creating ./local for prefix target..."
    mkdir -p local
fi
if [ ! -d packages ]; then
    mkdir -p packages
fi

bash scripts/build_libjpeg.sh
bash scripts/build_libsynchronous-frames.sh
bash scripts/build_mjpg-streamer.sh
