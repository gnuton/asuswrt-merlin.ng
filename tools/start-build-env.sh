#!/bin/bash

docker pull gnuton/asuswrt-merlin-toolchains-docker:latest-ubuntu-20_04
docker run -it --rm -v "$PWD:/build"  -u "docker:docker" \
       gnuton/asuswrt-merlin-toolchains-docker:latest-ubuntu-20_04 /bin/bash

