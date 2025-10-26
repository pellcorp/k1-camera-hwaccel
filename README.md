# MJPG Streamer for Creality OS with optional input_syncframes for NV12 encoder on the Creality Camera

Build Scripts to build `mjpg-streamer` with optional hardware acceleration (syncframe input plugin) on the K1 Series of
printers, which potentially also supports the nebula camera!?

## Credits

Thanks to zevaryx and Sekilsgs2 for all the initial work getting this build working

## Related Projects

The following forks of upstream projects (which are submodules in this one):

- https://github.com/pellcorp/jpeg-9d
- https://github.com/pellcorp/ingenic-mjpg-streamer

## Build Environment

### Install Docker

You can follow the instructions to get docker on Ubuntu:
https://docs.docker.com/engine/install/ubuntu/#install-using-the-repository

1. `sudo apt-get update && sudo apt-get install ca-certificates curl gnupg`
2. `sudo install -m 0755 -d /etc/apt/keyrings`
3. `curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg`
4. `sudo chmod a+r /etc/apt/keyrings/docker.gpg`
3. `echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null`
4. `sudo apt-get update`
5. `sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin`

## Building

Clone the repo (and submodules).

1. `git clone --recursive https://github.com/pellcorp/k1-mjpg-streamer && cd k1-mjpg-streamer`
2. `docker run -ti -v $PWD:$PWD pellcorp/k1-camera-build $PWD/build.sh`
3. The `build/mjpg-streamer.tar.gz` can be extracted to your printer, I suggest the /usr/data directory 

## Running it

A sample command assuming you extracted the `mjpg-streamer.tar.gz` to /usr/data

```
export LD_LIBRARY_PATH=/usr/data/mjpg-streamer/lib/:/usr/data/mjpg-streamer/lib/mjpg-streamer/
/usr/data/mjpg-streamer/bin/mjpg_streamer -i "/usr/data/mjpg-streamer/lib/mjpg-streamer/input_syncframes.so -r 1280x720 -m -camera_device0 $V4L_DEVICE" -o "/usr/data/mjpg-streamer/lib/mjpg-streamer/output_http.so -p 8080"
```
