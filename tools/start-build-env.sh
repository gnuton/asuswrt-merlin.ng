#!/bin/bash
docker pull gnuton/asuswrt-merlin-toolchains-docker
docker run -it --rm -v "$PWD:/build" gnuton/asuswrt-merlin-toolchains-docker /bin/bash

