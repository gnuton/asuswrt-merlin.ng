#!/bin/bash

docker pull gnuton/asuswrt-merlin-toolchains-docker:latest
docker run -it --rm -v "$PWD:/build"  -u "docker:docker" \
       gnuton/asuswrt-merlin-toolchains-docker:latest /bin/bash

