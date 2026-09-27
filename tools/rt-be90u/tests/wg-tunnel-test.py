#!/usr/bin/env python3
"""WG source writers + image ARM keys/import/routing; native host WG netlink/kernel.

This does not execute packaged rc service orchestration or the router's kernel.
The extracted functions are unmodified; their source hashes identify provenance.
"""
import argparse
import hashlib
import importlib.util
import os
from pathlib import Path
import re
import select
import shutil
import socket
import stat
import struct
import subprocess
import time

spec = importlib.util.spec_from_file_location('tunnel', '/tests/vpn-tunnel-test.py')
tunnel = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tunnel)
network = tunnel.network
run, ip = network.run, network.ip


def compile_config(root):
    directory = Path('/work/release/src/router/rc')
    source = (directory / 'wireguard.c').read_text()
    common = (directory / 'common.c').read_text()
    for name, data in [('wireguard.c', source), ('common.c', common)]:
        print(name + ' SHA-256:', hashlib.sha256(data.encode()).hexdigest(), flush=True)
    functions = [re.search(r'^#define WG_KEY_SIZE.*$', source, re.M)[0],
                 re.search(r'^char \*trim_r\(.*?\n}\n', common, re.M | re.S)[0]]
    for name in ('_wg_resolv_ep', '_wg_client_gen_conf', '_wg_server_gen_conf',
                 '_wg_server_get_endpoint', '_wg_server_gen_client_conf'):
        functions.append(re.search(r'^static [^\n]+?\b' + name + r'\(.*?\n}\n',
                                   source, re.M | re.S)[0])
    (root / 'harness/wg-config-source.inc').write_text('\n'.join(functions))
    run('/opt/openwrt-gcc750_musl1124.aarch64/bin/aarch64-openwrt-linux-musl-gcc',
        '-Wall', '-Wextra', '-Werror', '-Wl,-E', '-DTUFBE6500',
        '-I' + str(root / 'harness'), '-I/work/release/src/router/shared',
        '-I/work/release/src-qca-ipq53xx/include', '/tests/wg-daemon-config.c',
        '-L/firmware/usr/lib', '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib',
        '-lshared', '-lnvram', '-o', str(root / 'harness/wg-config'))
    native = root.parent / 'wg-native'
    shutil.copytree('/work/release/src/router/wireguard-tools/src', native)
    run('make', '-C', str(native), 'clean')
    run('make', '-C', str(native), '-j2', 'wg', 'CC=gcc')
    return str(native / 'wg')


def arm(root, *args, **kwargs):
    return run('chroot', str(root), '/qemu', *args, **kwargs).stdout.strip()


def configurations(root, wg, regressions):
    keys = {}
    for side in ('client', 'server'):
        keys[side] = arm(root, '/usr/sbin/wg', 'genkey')
        keys[side + '_pub'] = arm(root, '/usr/sbin/wg', 'pubkey', input=keys[side] + '\n')
        assert keys[side + '_pub'] == run(wg, 'pubkey', input=keys[side] + '\n').stdout.strip()
    keys['psk'] = arm(root, '/usr/sbin/wg', 'genpsk')
    print('PASS actual ARM WG key generation matches native public-key derivation', flush=True)

    def client(endpoint, alive, unset=False):
        config = ('[Interface]\nPrivateKey = ' + keys['client'] + '\nAddress = 10.9.0.2/24\n'
                  '[Peer]\nPublicKey = ' + keys['server_pub'] + '\nPresharedKey = ' + keys['psk']
                  + '\nAllowedIPs = 0.0.0.0/0,::/0\nEndpoint = ' + endpoint + '\n')
        if alive is not None:
            config += 'PersistentKeepalive = ' + alive + '\n'
        (root / 'tmp/provider.conf').write_text(config)
        env = dict(os.environ, ipv6_service='dhcp6')
        if unset:
            env['FIXTURE_UNSET_KEEPALIVE'] = '1'
        arm(root, '/harness/wg-config', 'client', env=env)
        run(wg, 'setconf', 'wgc1', str(root / 'tmp/client.conf'))
        applied = run(wg, 'show', 'wgc1', 'endpoints').stdout.split()[1]
        expected = '[2001:db8::2]:51820' if endpoint.startswith(('[', '2001:')) else endpoint
        assert applied == expected, (applied, expected)
        return run(wg, 'show', 'wgc1', 'persistent-keepalive').stdout.split()[1]

    failures = []
    cases = [('10.250.0.2:51820', '25', '25')]
    if regressions:
        cases += [('10.250.0.2:51820', '0', 'off'), ('10.250.0.2:51820', 'off', 'off'),
                  ('10.250.0.2:51820', None, 'off'), ('[2001:db8::2]:51820', '0', 'off'),
                  ('2001:db8::2:51820', '0', 'off')]
    for endpoint, alive, expected in cases:
        try:
            actual = client(endpoint, alive)
            assert actual == expected, (actual, expected)
            print('PASS import/source writer/native setconf endpoint=' + endpoint
                  + ' keepalive=' + str(alive), flush=True)
        except (AssertionError, RuntimeError) as error:
            failures.append((endpoint, alive, str(error)))
    assert not failures, failures
    assert client('10.250.0.2:51820', None, unset=True) == '25'
    print('PASS unset router keepalive retains the existing 25-second default', flush=True)
    client('10.250.0.2:51820', '0' if regressions else '25')
    env = dict(os.environ, wgs1_priv=keys['server'], wgs1_port='51820', wgs1_psk='1',
               wgs1_c1_enable='1', wgs1_c1_pub=keys['client_pub'], wgs1_c1_psk=keys['psk'],
               wgs1_c1_aips='10.9.0.2/32,192.0.2.0/24')
    arm(root, '/harness/wg-config', 'server', env=env)
    env.update(wgs1_pub=keys['server_pub'], wgs1_addr='10.9.0.1/24', wgs1_dns='1',
               wgs1_alive='0', wgs1_c1_priv=keys['client'], wgs1_c1_addr='10.9.0.2/24',
               wgs1_c1_caips='0.0.0.0/0', lan_ipaddr='10.250.0.2', wan0_ipaddr='10.250.0.2', sw_mode='1')
    arm(root, '/harness/wg-config', 'export', env=env)
    return keys


def transfer(peer, token, expected=True):
    code = '''import socket
with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as s:
 s.settimeout(TIMEOUT)
 s.bind(('203.0.113.99',12345))
 print('READY',flush=True)
 try:
  data,source=s.recvfrom(4096)
  assert source[0]=='192.0.2.10',source
  print(data.decode(),flush=True)
 except socket.timeout:
  print('NO-PACKET',flush=True)
'''
    code = code.replace('TIMEOUT', '25' if expected else '3')
    listener = subprocess.Popen([*peer, 'python3', '-u', '-c', code],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    captures = []
    for iface in ('uplink', 'wan0'):
        sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
        sock.bind((iface, 0))
        captures.append(sock)
    try:
        assert select.select([listener.stdout], [], [], 5)[0]
        assert listener.stdout.readline().strip() == 'READY'
        for attempt in range(48 if expected else 1):
            tunnel.inject(token)
            if select.select([listener.stdout], [], [], 0.5)[0]:
                break
        stdout, stderr = listener.communicate(timeout=5)
        assert listener.returncode == 0, stderr
        assert stdout.strip() == (token.decode() if expected else 'NO-PACKET'), stdout
        encrypted = []
        for sock in captures:
            while select.select([sock], [], [], 0)[0]:
                packet = sock.recv(65535)
                assert token not in packet, 'Plaintext escaped onto ' + sock.getsockname()[0]
                if (len(packet) >= 42 and packet[12:14] == b'\x08\x00' and packet[23] == 17
                        and 51820 in struct.unpack('!HH', packet[34:38])):
                    encrypted.append(packet)
        if expected:
            assert encrypted, 'No WG transport traffic captured'
    finally:
        tunnel.stop(listener)
        for sock in captures:
            sock.close()


def main(root, config_only, skip_regressions):
    network.prepare(root)
    for name in ('usr/sbin/wg', 'usr/lib/libshared.so'):
        print(name + ' SHA-256:', hashlib.sha256((Path('/firmware') / name).read_bytes()).hexdigest(), flush=True)
    network.setup_links()
    ip('link', 'del', 'wgc1')
    ip('link', 'add', 'wgc1', 'type', 'wireguard')
    for name, minor in [('urandom', 9), ('null', 3)]:
        path = root / 'dev' / name
        path.unlink(missing_ok=True)
        os.mknod(path, stat.S_IFCHR | 0o600, os.makedev(1, minor))
    wg = compile_config(root)
    keys = configurations(root, wg, not skip_regressions)
    if config_only:
        return
    holder = subprocess.Popen(['unshare', '-n', 'sleep', '300'])
    fixture = None
    try:
        for attempt in range(100):
            if os.readlink(f'/proc/{holder.pid}/ns/net') != os.readlink('/proc/self/ns/net'):
                break
            time.sleep(0.01)
        else:
            raise RuntimeError('Could not isolate WG peer namespace')
        peer = ('nsenter', '-t', str(holder.pid), '-n')
        ip('link', 'add', 'uplink', 'type', 'veth', 'peer', 'name', 'peer0')
        ip('link', 'set', 'peer0', 'netns', str(holder.pid))
        ip('addr', 'add', '10.250.0.1/24', 'dev', 'uplink')
        ip('link', 'set', 'uplink', 'up')
        for args in [('link', 'set', 'lo', 'up'), ('addr', 'add', '10.250.0.2/24', 'dev', 'peer0'),
                     ('link', 'set', 'peer0', 'up'), ('addr', 'add', '203.0.113.99/32', 'dev', 'lo'),
                     ('link', 'add', 'wgpeer', 'type', 'wireguard')]:
            run(*peer, 'ip', *args)
        run(*peer, wg, 'setconf', 'wgpeer', str(root / 'tmp/server.conf'))
        for args in [('addr', 'add', '10.9.0.1/24', 'dev', 'wgpeer'),
                     ('link', 'set', 'wgpeer', 'up'), ('route', 'add', '192.0.2.0/24', 'dev', 'wgpeer')]:
            run(*peer, 'ip', *args)
        ip('addr', 'add', '10.9.0.2/24', 'dev', 'wgc1')
        ip('link', 'set', 'wgc1', 'up')
        ip('route', 'add', 'default', 'dev', 'wgc1', 'table', 'wgc1')
        fixture = network.Fixture(root)
        fixture.rules('<1>encrypted>192.0.2.10>>WGC1')
        fixture.call('rules', 'wg')
        fixture.call('kill', 'wg')
        transfer(peer, b'RTBE90U-WG-INITIAL')
        assert int(run(wg, 'show', 'wgc1', 'latest-handshakes').stdout.split()[1]) > 0
        print('PASS encrypted WG LAN forwarding with ARM-imported/source-generated configs', flush=True)
        # A silent peer leaves the WG link up: protected traffic must still not use WAN.
        run(*peer, 'ip', 'link', 'set', 'wgpeer', 'down')
        transfer(peer, b'RTBE90U-WG-PEER-DOWN', False)
        run(*peer, 'ip', 'link', 'set', 'wgpeer', 'up')
        run(*peer, 'ip', 'route', 'replace', '192.0.2.0/24', 'dev', 'wgpeer')
        transfer(peer, b'RTBE90U-WG-PEER-RETURN')
        print('PASS peer outage prevents WAN escape; peer return restores encrypted forwarding', flush=True)
        fixture.call('clear', 'wg')
        ip('link', 'del', 'wgc1')
        fixture.sockets.pop('wgc1').close()
        fixture.route('192.0.2.10', '203.0.113.99', None)
        transfer(peer, b'RTBE90U-WG-DELETED', False)
        print('PASS interface removal and library policy cleanup retain the IPv4 kill switch', flush=True)
        ip('link', 'add', 'wgc1', 'type', 'wireguard')
        config = root / 'tmp/client.conf'
        original = config.read_text()
        wrong = arm(root, '/usr/sbin/wg', 'genpsk')
        config.write_text(original.replace(keys['psk'], wrong))
        run(wg, 'setconf', 'wgc1', str(config))
        ip('addr', 'add', '10.9.0.2/24', 'dev', 'wgc1')
        ip('link', 'set', 'wgc1', 'up')
        ip('route', 'add', 'default', 'dev', 'wgc1', 'table', 'wgc1')
        fixture.call('rules', 'wg')
        transfer(peer, b'RTBE90U-WG-WRONG-PSK', False)
        assert int(run(wg, 'show', 'wgc1', 'latest-handshakes').stdout.split()[1]) == 0
        print('PASS incorrect PSK fails the handshake without WAN escape', flush=True)
        # Remove stale handshake state before restoring the correct peer configuration.
        run(wg, 'set', 'wgc1', 'peer', keys['server_pub'], 'remove')
        config.write_text(original)
        run(wg, 'setconf', 'wgc1', str(config))
        transfer(peer, b'RTBE90U-WG-RESTART')
        print('PASS restored configuration reconnects and forwards encrypted packets', flush=True)
        exported = (root / 'tmp/export.conf').read_text()
        assert 'Endpoint = 10.250.0.2:51820\n' in exported
        assert 'PersistentKeepalive' not in exported
        assert 'Address = 10.9.0.2/24\n' in exported and 'DNS = 10.9.0.1\n' in exported
        # Address and DNS are wg-quick directives; the fixture already owns its addresses/routes.
        stripped = '\n'.join(line for line in exported.splitlines()
                             if not line.startswith(('Address =', 'DNS =')))
        config.write_text(stripped + '\n')
        run(wg, 'set', 'wgc1', 'peer', keys['server_pub'], 'remove')
        run(wg, 'setconf', 'wgc1', str(config))
        assert run(wg, 'show', 'wgc1', 'persistent-keepalive').stdout.split()[1] == 'off'
        transfer(peer, b'RTBE90U-WG-EXPORTED')
        print('PASS server-exported profile establishes encrypted forwarding with keepalive off', flush=True)
    finally:
        if fixture:
            for sock in fixture.sockets.values():
                sock.close()
        tunnel.stop(holder)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('root', type=Path)
    parser.add_argument('--config-only', action='store_true')
    parser.add_argument('--skip-config-regressions', action='store_true')
    args = parser.parse_args()
    main(args.root, args.config_only, args.skip_config_regressions)
