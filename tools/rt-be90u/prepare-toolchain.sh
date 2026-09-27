#!/bin/bash
# Provision the builder from an already available, pinned toolchain checkout.
# This script never fetches Git objects or packages. Provision Dockerfile.deps
# separately, or supply an existing compatible Ubuntu 20.04 builder image.
set -euo pipefail
if [ "$#" -lt 1 ] || [ "$#" -gt 3 ]; then
	printf 'Usage: %s QCA_TOOLCHAIN_CHECKOUT [IMAGE_TAG [BUILD_DEPS_IMAGE]]\n' "$0" >&2
	exit 1
fi
test "$(uname -sm)" = 'Linux x86_64'
script_dir=$(cd -- "$(dirname -- "$0")" && pwd)
toolchain_dir=$(cd -- "$1" && pwd)
image_tag=${2:-rt-be90u-build:3006}
deps_image=${3:-rt-be90u-build-deps:20.04}
docker image inspect "$deps_image" >/dev/null
revision=5e5b8ae593edb9c3f0711c56ef7fe73e113506d0
package=openwrt-gcc750_musl1124.aarch64
export GIT_NO_LAZY_FETCH=1
test "$(git -C "$toolchain_dir" rev-parse HEAD)" = "$revision"
git -C "$toolchain_dir" diff --quiet HEAD -- "$package"
test -z "$(git -C "$toolchain_dir" ls-files --others --exclude-standard -- "$package")"
context_dir=$(mktemp -d)
trap 'rm -rf -- "$context_dir"' EXIT
# Archive the subtree itself: some Git versions preload unrelated blobs when
# archiving a whole commit with a path filter, breaking sparse/partial clones.
git -C "$toolchain_dir" archive --prefix="$package/" "$revision:$package" | tar -x -C "$context_dir"
cp "$script_dir/Dockerfile" "$context_dir/Dockerfile"
docker build --network=none --build-arg "BUILD_DEPS_IMAGE=$deps_image" -t "$image_tag" "$context_dir"
