# Dockerfile

For building k1 mjpg steamer, best **not** to use it for building anything else.

## Publishing docker file

```
docker build . -t pellcorp/k1-camera-build
docker login
docker push pellcorp/k1-camera-build
```
