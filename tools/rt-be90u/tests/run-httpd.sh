#!/bin/sh
# Test-only HTTP server. Use an internal Docker network and a localhost SSH tunnel.
set -eu
test_dir=$(mktemp -d)
trap 'rm -rf -- "$test_dir"' EXIT
mkdir "$test_dir/root"
cp -a /firmware/. "$test_dir/root/"
printf '\n<!-- RTBE90U_HTTPD_FIXTURE -->\n' >> "$test_dir/root/www/Main_Login.asp"
cp /usr/bin/qemu-aarch64-static "$test_dir/root/qemu-aarch64-static"
mkdir -p "$test_dir/root/tmp/etc" "$test_dir/root/tmp/var/run" \
    "$test_dir/root/tmp/var/log" "$test_dir/root/jffs/openvpn" "$test_dir/root/dev" "$test_dir/root/proc"
cp -a "$test_dir/root/rom/etc/." "$test_dir/root/tmp/etc/"
touch "$test_dir/root/dev/null"
printf 'admin:%s:0:0:99999:7:::\n' "$(openssl passwd -6 -salt rtbe90utest rtbe90u-fixture)" \
    > "$test_dir/root/tmp/etc/shadow"
printf '%s\n' 'admin:x:0:0:Test:/tmp/home/root:/bin/sh' > "$test_dir/root/tmp/etc/passwd"
/opt/openwrt-gcc750_musl1124.aarch64/bin/aarch64-openwrt-linux-musl-gcc \
    -Wall -Wextra -Werror -shared -fPIC /tests/httpd-nvram.c \
    -o "$test_dir/root/tmp/httpd-nvram.so"
python3 - "$test_dir/root" <<'PY'
import os
import struct
import sys
import zlib
from pathlib import Path

# The tmpfs is nodev. Supply test entropy as files instead of device nodes.
for name in ('random', 'urandom'):
    (Path(sys.argv[1]) / 'dev' / name).write_bytes(os.urandom(1024 * 1024))
(Path(sys.argv[1]) / 'proc/uptime').write_text(Path('/proc/uptime').read_text())
for unit in (1, 2):
    directory = Path(sys.argv[1]) / ('etc/openvpn/server%d' % unit)
    directory.mkdir(parents=True, exist_ok=True)
    (directory / 'client.ovpn').write_text(
        '# RTBE90U_EXPORT_FIXTURE_%d\nclient\nremote 192.0.2.%d 1194\n' % (unit, unit))
directory = Path(sys.argv[1]) / 'etc/wg'
directory.mkdir(parents=True, exist_ok=True)
def png_chunk(kind, data):
    return struct.pack('!I', len(data)) + kind + data + struct.pack('!I', zlib.crc32(kind + data))
for unit in (1, 2):
    # Synthetic per-peer artifacts stand in for rc output, never real credentials.
    marker = ('RTBE90U_WG_PRIVATE_FIXTURE_%d' % unit).encode()
    (directory / ('server1_client%d.conf' % unit)).write_bytes(marker + b'\n')
    png = b'\x89PNG\r\n\x1a\n' + png_chunk(b'IHDR', struct.pack('!IIBBBBB', 1, 1, 8, 2, 0, 0, 0))
    png += png_chunk(b'tEXt', b'fixture\0' + marker)
    png += png_chunk(b'IDAT', zlib.compress(b'\0\0\0\0')) + png_chunk(b'IEND', b'')
    (directory / ('server1_client%d.png' % unit)).write_bytes(png)

settings = {
    'productid': 'TUF-BE9400', 'odmpid': 'RT-BE90U',
    'firmver': '3.0.0.6', 'buildno': '102', 'extendno': 'rtbe90u-http-test',
    'sw_mode': '1', 'x_Setting': '1', 'p_Setting': '1', 'w_Setting': '1',
    'ASUS_NEW_EULA': '1', 'ASUS_NEW_EULA_time': '2026-09-25T00:00:00Z',
    # Synthetic policy version/time; SDK key names from webapi_get_b(3/4).
    '68feaaa3': '2', '767cd197': '2026-09-25T00:00:00Z',
    'TM_EULA': '0',
    'sdn_rl': '<0>TEST>1>0>0>0>0>0>0>0>0>0>0>0>0>0>0>0>0>WEB>0>0>0',
    'http_username': 'admin', 'http_passwd': 'rtbe90u-fixture',
    'lan_ifname': 'eth0', 'lan_ipaddr': '127.0.0.1', 'lan_netmask': '255.0.0.0',
    'http_lanport': '8080', 'http_enable': '0', 'captcha_enable': '0',
    'wl_ifnames': 'ath0 ath1 ath2', 'wl0_nband': '2', 'wl1_nband': '1', 'wl2_nband': '4',
    'rc_support': '2.4G 5G 6G mssid update usbX2 openvpnd vpnc vpn_fusion wireguard mtlancfg odmpid ipv6',
    'vpn_client_unit': '1', 'vpn_server_unit': '1', 'wgc_unit': '1', 'wgs_unit': '1', 'wgsc_unit': '1',
    'vpn_client1_state': '0', 'vpn_client1_errno': '0',
    'vpn_server1_state': '0', 'vpn_server1_errno': '0',
}
os.environ.update({'RTBE90U_NVRAM_' + key: value for key, value in settings.items()})
os.chroot(sys.argv[1])
os.chdir('/www')
os.execv('/qemu-aarch64-static', ['/qemu-aarch64-static', '-E',
         'LD_PRELOAD=/tmp/httpd-nvram.so', '/usr/sbin/httpd', '-i', 'eth0', '-p', '8080'])
PY
