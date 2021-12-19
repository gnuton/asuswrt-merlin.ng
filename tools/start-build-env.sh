#!/bin/bash
docker pull gnuton/asuswrt-merlin-toolchains-docker:latest-ubuntu-20_04
docker run -it --rm -v "$PWD:/build"  -u $(id -u ${USER}):$(id -g ${USER}) \
       gnuton/asuswrt-merlin-toolchains-docker:latest-ubuntu-20_04 /bin/bash
