#!/bin/sh
# Test image at /firmware, pinned downloads at /packages, tests at /tests.
# Run only in the network-disabled rt-be90u-network-test container.
set -eu
test -f /.dockerenv
python3 - <<'PY'
import hashlib
import json
from pathlib import Path

manifest = json.loads(Path('/tests/entware-install-fixture.json').read_text())
expected_packages = {name for name in manifest['files'] if name.endswith('.ipk')}
if {path.name for path in Path('/packages').glob('*.ipk')} != expected_packages:
    raise SystemExit('Unexpected package set')
for name, expected in manifest['files'].items():
    if hashlib.sha256((Path('/packages') / name).read_bytes()).hexdigest() != expected:
        raise SystemExit('Checksum mismatch: ' + name)
PY
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
mkdir "$test_dir/root"
cp -a /firmware/. "$test_dir/root/"
cp /usr/bin/qemu-aarch64-static "$test_dir/root/qemu-aarch64-static"
# Model persistent USB storage with a directory; no mount or physical drive.
# Keep the firmware's /opt -> tmp/opt link and point tmp/opt at this directory.
mkdir -p "$test_dir/root/tmp/mnt/fixture-usb/entware/bin" "$test_dir/root/tmp/etc"
mkdir -p "$test_dir/root/dev"
touch "$test_dir/root/dev/null"
# A relative target also resolves within the fixture during host-side setup.
ln -s mnt/fixture-usb/entware "$test_dir/root/tmp/opt"
cp -a "$test_dir/root/rom/etc/." "$test_dir/root/tmp/etc/"
cp /packages/installer/opkg "$test_dir/root/opt/bin/opkg"
chmod 755 "$test_dir/root/opt/bin/opkg"
mkdir -p "$test_dir/root/opt/etc" "$test_dir/root/opt/tmp" "$test_dir/root/opt/var/lock"
printf '%s\n' 'arch all 1' 'arch aarch64-3.10 10' > "$test_dir/root/opt/etc/opkg.conf"
for applet in sh cat sort ln mkdir; do
    rm -f "$test_dir/root/bin/$applet"
    cp /bin/busybox "$test_dir/root/bin/$applet"
done
rm "$test_dir/root/bin/gzip"
cat > "$test_dir/root/bin/gzip" <<'WRAPPER'
#!/bin/sh
exec /qemu-aarch64-static /bin/busybox gzip "$@"
WRAPPER
chmod 755 "$test_dir/root/bin/gzip"
mkdir -p "$test_dir/root/packages"
cp /packages/*.ipk "$test_dir/root/packages/"
# Defer scripts until explicit emulator launchers are in place. No binfmt setup.
chroot "$test_dir/root" /qemu-aarch64-static /opt/bin/opkg --offline-root / install /packages/*.ipk
for name in localedef.new locale.new grep; do
    # grep is an opkg alternative symlink. Keep the installed target intact.
    source="$test_dir/root/opt/bin/$name"
    if [ -L "$source" ]; then
        target=$(readlink "$source")
        case "$target" in
            /opt/*) source="$test_dir/root$target" ;;
            *) printf 'Unexpected alternative: %s\n' "$target" >&2; exit 1 ;;
        esac
    fi
    cp "$source" "$test_dir/root/opt/bin/$name.arm"
    rm "$test_dir/root/opt/bin/$name"
    printf '#!/bin/sh\nexec /qemu-aarch64-static /opt/bin/%s.arm "$@"\n' "$name" > "$test_dir/root/opt/bin/$name"
    chmod 755 "$test_dir/root/opt/bin/$name"
done
# opkg offline mode marks packages configured without executing postinst.
# Run the installed scripts explicitly after adding the QEMU dispatchers.
for script in "$test_dir/root"/opt/lib/opkg/info/*.postinst; do
    PATH=/opt/bin:/opt/sbin:/usr/sbin:/usr/bin:/sbin:/bin \
        chroot "$test_dir/root" /bin/sh "${script#"$test_dir/root"}"
done
test -s "$test_dir/root/opt/usr/lib/locale/locale-archive"
test -L "$test_dir/root/opt/bin/sh"
chroot "$test_dir/root" /qemu-aarch64-static /opt/bin/opkg list-installed
result=$(chroot "$test_dir/root" /qemu-aarch64-static /opt/bin/opkg print-architecture)
printf '%s\n' "$result"
printf '%s\n' "$result" | grep -qx 'arch aarch64-3.10 10'
chroot "$test_dir/root" /qemu-aarch64-static /opt/libexec/find-gnu --version
mkdir "$test_dir/root/opt/rt-be90u-fixture"
touch "$test_dir/root/opt/rt-be90u-fixture/example"
result=$(chroot "$test_dir/root" /qemu-aarch64-static /opt/libexec/find-gnu \
    /opt/rt-be90u-fixture -type f -name example -print)
test "$result" = /opt/rt-be90u-fixture/example
# The real rc.unslung invokes GNU find; dispatch its ARM executable explicitly.
mv "$test_dir/root/opt/libexec/find-gnu" "$test_dir/root/opt/libexec/find-gnu.arm"
cat > "$test_dir/root/opt/libexec/find-gnu" <<'WRAPPER'
#!/bin/sh
exec /qemu-aarch64-static /opt/libexec/find-gnu.arm "$@"
WRAPPER
chmod 755 "$test_dir/root/opt/libexec/find-gnu"
mkdir -p "$test_dir/root/harness" "$test_dir/root/jffs/scripts" "$test_dir/root/opt/var/run"
/opt/openwrt-gcc750_musl1124.aarch64/bin/aarch64-openwrt-linux-musl-gcc \
    -Wall -Wextra -Werror -Wl,-E /tests/extension-hook-driver.c \
    -L/firmware/usr/lib -Wl,-rpath-link,/firmware/usr/lib:/firmware/lib -lshared -lnvram \
    -o "$test_dir/root/harness/driver"
cat > "$test_dir/root/jffs/scripts/services-start" <<'HOOK'
#!/bin/sh
/opt/etc/init.d/rc.unslung start
HOOK
cat > "$test_dir/root/jffs/scripts/services-stop" <<'HOOK'
#!/bin/sh
/opt/etc/init.d/rc.unslung stop
HOOK
cat > "$test_dir/root/opt/etc/init.d/S10fixture.sh" <<'SERVICE'
#!/bin/sh
echo "10 $1" >> /opt/var/service-order
case "$1" in
    start)
        /qemu-aarch64-static /harness/driver daemon &
        echo "$!" > /opt/var/run/fixture.pid
        ;;
    stop)
        kill "$(cat /opt/var/run/fixture.pid)"
        ;;
esac
SERVICE
cat > "$test_dir/root/opt/etc/init.d/S20fixture.sh" <<'SERVICE'
#!/bin/sh
echo "20 $1" >> /opt/var/service-order
SERVICE
chmod 755 "$test_dir/root/jffs/scripts/"* "$test_dir/root/opt/etc/init.d/"S*fixture.sh
chroot "$test_dir/root" /qemu-aarch64-static /harness/driver 0 services-start
test ! -e "$test_dir/root/opt/var/service-order"
chroot "$test_dir/root" /qemu-aarch64-static /harness/driver 1 services-start
service_pid=$(cat "$test_dir/root/opt/var/run/fixture.pid")
python3 - "$test_dir/root/opt/var/run/fixture.ready" "$service_pid" <<'PY'
from pathlib import Path
import os
import sys
import time

ready = Path(sys.argv[1])
for attempt in range(100):
    if ready.exists() and ready.read_text().strip() == sys.argv[2]:
        time.sleep(0.1)
        os.kill(int(sys.argv[2]), 0)
        assert not (Path('/proc') / sys.argv[2] / 'stat').read_text().split(') ', 1)[1].startswith('Z ')
        break
    time.sleep(0.02)
else:
    raise SystemExit('ARM fixture daemon did not reach readiness')
PY
chroot "$test_dir/root" /qemu-aarch64-static /harness/driver 1 services-stop
python3 - "$test_dir/root/opt" "$service_pid" <<'PY'
from pathlib import Path
import sys
import time

storage = Path(sys.argv[1])
assert storage.resolve().name == 'entware'
assert (storage / 'var/service-order').read_text().splitlines() == ['10 start', '20 start', '20 stop', '10 stop']
for attempt in range(50):
    process = Path('/proc') / sys.argv[2] / 'stat'
    if not process.exists() or process.read_text().split(') ', 1)[1].startswith('Z '):
        break
    time.sleep(0.02)
else:
    raise SystemExit('Fixture service still running after stop hook')
assert (storage / 'rt-be90u-fixture/example').exists()
print('Entware package install, explicit postinst, storage layout, real hooks and rc.unslung ordering passed')
print('Physical USB, firmware shell subprocess execution and boot/hotplug ordering remain untested')
PY
