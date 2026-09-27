ARG BASE_IMAGE=rt-be90u-build:58138
FROM ${BASE_IMAGE}
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends qemu-user-static \
    && rm -rf /var/lib/apt/lists/*
