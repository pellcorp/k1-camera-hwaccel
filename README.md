# K1 Camera with Hardware Acceleration

Build Scripts to build `mjpg-streamer` with hardware acceleration on the K1/K1 Max

## Prerequisites

- `cmake`
- `make`
- `wget`

On Ubuntu, run:

`sudo apt-get update && sudo apt-get install build-essential wget cmake`

## Building the packages

Simply run `bash build.sh`. The build scripts will automatically download and compile:

- The required `gcc` toolchain
- `libjpeg`
- `libsynchronous-frames`
- `mjpg-streamer`

The compiled files will output into `local/` for use on systems