# MJPG Streamer for Creality OS with optional input_syncframes for NV12 encoder on the Creality Camera

Build Scripts to build `mjpg-streamer` with optional hardware acceleration (syncframe input plugin) on the K1/K1 Max

## Using Docker

```bash
docker run -ti -v $PWD:$PWD pellcorp/k1-camera-build $PWD/build.sh
```

This will produce a `mjpg-streamer.tar.gz` in the build/ directory
