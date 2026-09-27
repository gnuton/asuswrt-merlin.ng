ARG BUILD_DEPS_IMAGE=rt-be90u-build-deps:20.04
FROM ${BUILD_DEPS_IMAGE}

# ASUS's 58138 archive contains LFS pointers for its toolchains.
# Supply an unpacked toolchain; provenance is recorded in docs/rt-be90u-integration.md.
COPY openwrt-gcc750_musl1124.aarch64 /opt/openwrt-gcc750_musl1124.aarch64
RUN /opt/openwrt-gcc750_musl1124.aarch64/bin/aarch64-openwrt-linux-musl-gcc --version

ENV LC_ALL=C
ENV PATH=/opt/openwrt-gcc750_musl1124.aarch64/bin:$PATH
WORKDIR /work/release/src-qca-ipq53xx
CMD ["make", "rt-be90u"]
