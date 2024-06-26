#! /bin/bash

set -ex

imagename="armbuild"
build=
tag=latest
deployPath="$HOME/cmpt433/public/myApps"

print_usage() {
    cat <<EOF
usage: dockerbuild.sh [--image-name {imagename}] [--tag {tag}] [--build]
                      [--deploy-to {/deploy/path}]

Cross-compiles the current working directory. If the build copies to the public
directory, ensure it gets copied to the given deployment path.

Accepts the following arguments:

    --image-name {image}       : The name of the docker image to build/use.
    --tag {tag}                : The tag of the docker image to build/use.
    --build                    : If present, build the docker image before
                                 compiling.
    --deploy-to {/deploy/path} : Local deploy path to mount into the container.
EOF
}

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
            "--deploy-to")
                shift
                deployPath="$1"
            ;;
            "--usage"|"-h"|"--help")
                print_usage
                exit 0
            ;;
            *)
                echo "Error: invalid switch $1"
                print_usage
                exit 1
        esac
        shift
    done
fi

if [[ $build ]]; then
    docker build -t "$imagename":"$tag" "$(dirname "$0")/env/docker"
fi

docker run -t \
    -v .:/srcdir \
    -v "$deployPath":/cmpt433/public/myApps \
    --rm \
    -u "$UID:$GID" \
    "$imagename":"$tag" \
    /bin/sh -c "cd /srcdir && cmake -S . -DCMAKE_TOOLCHAIN_FILE=/srcdir/bbg.toolchain.cmake -B build && cmake --build build/"
