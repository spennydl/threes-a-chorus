FROM debian:11.8

RUN apt update -y && apt upgrade -y
RUN apt install -y gcc make cmake
RUN apt install -y gcc-arm-linux-gnueabihf
RUN apt install -y binutils-arm-linux-gnueabihf
RUN apt install -y libasound2-dev

VOLUME ["/srcdir"]
VOLUME ["/cmpt433/public/myApps"]

CMD arm-linux-gnueabihf-gcc --version
