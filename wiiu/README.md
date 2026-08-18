## Building

For building you need:
- [wut](https://github.com/decaf-emu/wut)
- [wups](https://github.com/wiiu-env/WiiUPluginSystem)
- [libmocha](https://github.com/wiiu-env/libmocha)

## Building using the Dockerfile
It's possible to use a Docker image for building. This way you don't need anything installed on your host system.

```bash
# Build docker image (only needed once)
docker build . -t richpresence-plugin-builder

# make 
docker run -it --rm -v ${PWD}:/project richpresence-plugin-builder make

# make clean
docker run -it --rm -v ${PWD}:/project richpresence-plugin-builder make clean
```
