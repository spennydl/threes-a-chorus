#! /bin/bash

set -ex

imagename="armbuild"
build=
tag=latest

if [[ $# -gt 0 ]]; then
    while (( $# )); do
        case "$1" in
            "--image-name")
                shift
                imagename="$1"
            ;;
            "--tag")
                shift
                tag="$1"
            ;;
            "--build")
                build=1
            ;;
            *)
                echo "Error: invalid switch $1"
                exit 1
        esac
        shift
    done
fi

if [[ $build ]]; then
    docker build -t "$imagename":"$tag" ./buildenv
fi

docker run -t \
    -v .:/srcdir \
    -v /home/spennydl/beaglebone:/cmpt433/public/myApps \
    --rm \
    -u "$UID:$GID" \
    "$imagename":"$tag" \
    /bin/sh -c "cd /srcdir && cmake -S . -DCMAKE_TOOLCHAIN_FILE=/srcdir/bbg.toolchain.cmake -B build && cmake --build build/"