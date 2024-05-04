# Building Three's a Chorus

## Prerequisites

There are two ways to build the project: either directly on the host using CMake
and the GCC arm-linux-gnueabihf cross-compiler, or using a dockerized
cross-compilation environment.

If compiling directly on the host you should have:
- A debian 11 environment (or a Linux environment with compatible libc)
- CMake
- The arm-linux-gnueabihf-gcc compiler

If cross-compiling using Docker, you can use any host OS and you only need
Docker installed.

## Build with CMake

Both the client and server can be built with CMake.

From either the server or the the client directory:

```sh
$ cmake -S . -DCMAKE_TOOLCHAIN_FILE=./bbg.toolchain.cmake -B build
$ cmake --build build
```

The build will copy the resulting executable to your `~/cmpt433/public/myApps`
directory.

## Build with Docker

A script called `dockerbuild.sh` is provided that can both build the
cross-compilation image and cross-compile the project in a container.

The script takes the following arguments:

- `--image-name {IMAGE_NAME}` :: The name of the docker image to build/use. Defaults
  to `armbuild`.
- `--tag {IMAGE_TAG}` :: The tag of the image to build/use. Defaults to
  `latest`.
- `--deploy-to {DEPLOY_PATH}` :: The host path where the resulting executable
  will be copied to after building. Defaults to `$HOME/cmpt433/public/myApps`.
- `--build` :: If this switch is present, a docker image will be built with the
  specified image name and tag. The built image will then be used to
  cross-compile the project. The Dockerfile used to build the image is located
  in the repository under `env/docker`.
  
To build the image and cross-compile the client, change to the `client`
directory and run:

```sh
$ ../dockerbuild.sh --build
```

