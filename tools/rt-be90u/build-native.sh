#!/bin/sh
# Called by the model Makefile. All build output stays under the platform tree.
set -eu
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$script_dir/../.." && pwd)
platform_dir=$repo_dir/release/src-qca-ipq53xx
make_command=${1:-make}
compiler=/opt/openwrt-gcc750_musl1124.aarch64/bin/aarch64-openwrt-linux-musl-gcc
test -x "$compiler" || { printf 'Missing IPQ53xx toolchain: %s\n' "$compiler" >&2; exit 1; }
test "$("$compiler" -dumpmachine)" = aarch64-openwrt-linux-musl
export PATH="$(dirname -- "$compiler"):$PATH"
export LC_ALL=C
export GIT_NO_LAZY_FETCH=1
exec 9>"$platform_dir/.build.lock"
flock 9
python3 "$script_dir/prepare-native.py"
source_dir=$platform_dir/.build/source
# Vendor release scripts probe Git and create tags when they find a repository.
# Keep those probes inside the disposable SDK tree, away from this checkout.
export GIT_CEILING_DIRECTORIES="$platform_dir/.build"
"$make_command" -C "$source_dir/release/src-qca-ipq53xx" tuf-be6500
# The board's required uImage identity is TUF-BE9400; RT-BE90U is its ODM name.
# Never collect the vendor's generic symlink, which points at the BE6500 image.
version=$(sed -n 's/^EXTENDNO=//p' "$source_dir/release/src/router/extendno.conf")
case "$version" in
    ''|*[!a-zA-Z0-9._-]*) printf 'Invalid experimental version\n' >&2; exit 1 ;;
esac
candidate=$source_dir/release/src-qca-ipq53xx/image/TUF-BE9400_3.0.0.6_102_$version.trx
python3 "$script_dir/check-image.py" "$candidate"
mkdir -p "$platform_dir/image"
cp -- "$candidate" "$platform_dir/image/$(basename -- "$candidate")"
printf 'RT-BE90U experimental image: %s/image/%s\n' "$platform_dir" "$(basename -- "$candidate")"
