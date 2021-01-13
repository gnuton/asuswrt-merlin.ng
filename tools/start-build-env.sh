#!/bin/bash
docker pull gnuton/asuswrt-merlin-toolchains-docker:latest-ubuntu-20_04
docker run -it --rm -v "$PWD:/root/project" gnuton/asuswrt-merlin-toolchains-docker:latest-ubuntu-20_04 /bin/bash

