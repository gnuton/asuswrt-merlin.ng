#!/usr/bin/env python3
"""Actual firmware OpenVPN daemons and encrypted packets in isolated namespaces."""
import argparse
import importlib.util
import os
from pathlib import Path
import select
import socket
import stat
import struct
import subprocess
import time

spec = importlib.util.spec_from_file_location('network', '/tests/vpn-network-test.py')
network = importlib.util.module_from_spec(spec)
spec.loader.exec_module(network)
run, ip = network.run, network.ip


def await_interface(name, prefix=()):
    for attempt in range(100):
        if run(*prefix, 'ip', 'link', 'show', name, check=False).returncode == 0:
            return
        time.sleep(0.05)
    raise RuntimeError('OpenVPN did not create ' + name)


def stop(process):
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)


def inject(token):
    source, dest = socket.inet_aton('192.0.2.10'), socket.inet_aton('203.0.113.99')
    udp = struct.pack('!HHHH', 31000, 12345, 8 + len(token), 0) + token
    header = struct.pack('!BBHHHBBH4s4s', 0x45, 0, 20 + len(udp), 1, 0, 64, 17, 0, source, dest)
    header = header[:10] + struct.pack('!H', network.checksum(header)) + header[12:]
    mac = bytes.fromhex(Path('/sys/class/net/br0/address').read_text().strip().replace(':', ''))
    with socket.socket(socket.AF_PACKET, socket.SOCK_RAW) as sender:
        sender.bind(('lanpeer', 0))
        sender.send(mac + b'\x02\x00\x00\x00\x00\x10\x08\x00' + header + udp)


def transfer(peer, token):
    code = '''import socket
with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as s:
 s.settimeout(8)
 s.bind(('203.0.113.99',12345))
 print('READY',flush=True)
 data,source=s.recvfrom(4096)
 assert source[0]=='192.0.2.10',source
 print(data.decode(),flush=True)
'''
    listener = subprocess.Popen([*peer, 'python3', '-u', '-c', code],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    outer = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
    outer.bind(('uplink', 0))
    try:
        assert select.select([listener.stdout], [], [], 5)[0], 'Peer listener failed to start'
        assert listener.stdout.readline().strip() == 'READY'
        # Static-key peers may need a moment to establish their UDP association.
        for attempt in range(3):
            inject(token)
            if select.select([listener.stdout], [], [], 1)[0]:
                break
        stdout, stderr = listener.communicate(timeout=10)
        assert listener.returncode == 0 and stdout.strip() == token.decode(), (stdout, stderr)
        packets = []
        while select.select([outer], [], [], 0)[0]:
            packet = outer.recv(65535)
            if (len(packet) >= 42 and packet[12:14] == b'\x08\x00' and packet[23] == 17
                    and 1194 in struct.unpack('!HH', packet[34:38])):
                packets.append(packet)
        assert packets, 'No OpenVPN UDP traffic observed on the transport link'
        assert all(token not in packet for packet in packets), 'Plaintext test payload escaped onto the transport'
    finally:
        stop(listener)
        outer.close()


def generate_config(root, mode):
    certs = root / 'tmp/certs'
    certs.mkdir()
    if mode == 'tls-crypt-v2':
        run('chroot', str(root), '/qemu', '/usr/sbin/openvpn', '--genkey',
            'tls-crypt-v2-server', '/tmp/tlscrypt-server.key')
        run('chroot', str(root), '/qemu', '/usr/sbin/openvpn', '--tls-crypt-v2',
            '/tmp/tlscrypt-server.key', '--genkey', 'tls-crypt-v2-client', '/tmp/tlscrypt-client.key')
    if mode != 'static':
        run('openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-days', '1',
            '-subj', '/CN=RTBE90U Fixture CA', '-keyout', str(certs / 'ca.key'),
            '-out', str(certs / 'ca.crt'))
        for side in ('server', 'client'):
            run('openssl', 'req', '-newkey', 'rsa:2048', '-nodes',
                '-subj', '/CN=rtbe90u-test-' + side, '-keyout', str(certs / (side + '.key')),
                '-out', str(certs / (side + '.csr')))
            extension = certs / (side + '.ext')
            extension.write_text('basicConstraints=CA:FALSE\nkeyUsage=digitalSignature,keyEncipherment\n'
                                 'extendedKeyUsage=' + side + 'Auth\n')
            run('openssl', 'x509', '-req', '-in', str(certs / (side + '.csr')),
                '-CA', str(certs / 'ca.crt'), '-CAkey', str(certs / 'ca.key'),
                '-CAcreateserial', '-days', '1', '-extfile', str(extension),
                '-out', str(certs / (side + '.crt')))
    for side in ('server', 'client'):
        (root / ('etc/openvpn/' + side + '1')).mkdir(parents=True, exist_ok=True)
    for name in ('config', 'event'):
        run('/opt/openwrt-gcc750_musl1124.aarch64/bin/aarch64-openwrt-linux-musl-gcc',
            '-Wall', '-Wextra', '-Werror', '-Wl,-E', '-I/work/release/src/router/libovpn',
            '-I/work/release/src/router/shared',
            '/tests/vpn-daemon-' + name + '.c', '-L/firmware/usr/lib',
            '-Wl,-rpath-link,/firmware/usr/lib:/firmware/lib', '-lovpn',
            '-o', str(root / ('harness/' + name)))
    print(run('chroot', str(root), '/qemu', '/harness/config', mode).stdout, end='', flush=True)
    exported = root / 'etc/openvpn/server1/client.ovpn'
    assert 'paste client' not in exported.read_text(), 'Valid stored client certificate was not exported'
    run('chroot', str(root), '/qemu', '/usr/sbin/openvpn', '--config',
        '/etc/openvpn/server1/client.ovpn', '--show-ciphers')
    print('PASS generated client export embeds stored keys and is accepted by the daemon parser', flush=True)
    for side in ('server', 'client'):
        directory = root / ('etc/openvpn/' + side + '1')
        config = (directory / 'config.ovpn').read_text()
        # Run in foreground and assign addresses using native iproute2. Native
        # scripts dispatch the actual ARM library hooks without global binfmt.
        skipped = {'daemon'}
        adapted = [line for line in config.splitlines() if not line.split() or line.split()[0] not in skipped]
        (directory / 'fixture.ovpn').write_text('\n'.join(adapted) + '\nifconfig-noexec\nroute-noexec\n')
        number = (1 if side == 'server' else 2) if mode != 'static' else (2 if side == 'server' else 1)
        for event in ('up', 'down', 'route-up', 'route-pre-down'):
            script = directory / ('ovpn-' + event)
            script.unlink(missing_ok=True)
            content = '#!/bin/sh\nset -eu\n'
            # OpenVPN intentionally passes a restricted environment to scripts.
            # Supply the same synthetic NVRAM at this process boundary.
            content += ('export lan_ifname=br0 vpn_clientx_eas=1, vpn_client1_rgw=2 '
                        'vpn_client1_enforce=1 vpn_client1_verb=3 vpn_client1_adns=0 '
                        'vpn_client1_state=2 jffs2_scripts=0\n')
            if event == 'up':
                content += '/usr/sbin/ip addr replace 10.9.0.%d/24 dev "$dev"\n' % number
                content += '/usr/sbin/ip link set "$dev" up\n'
            content += 'exec /qemu /harness/event ' + event + ' "$@"\n'
            script.write_text(content)
            script.chmod(0o755)


def exported_tunnel(root, mode, peer, processes, logs):
    # tls-crypt-v2 is a custom provider fixture, not a server UI export mode.
    if mode == 'tls-crypt-v2':
        return
    directory = root / 'tmp/export'
    directory.mkdir()
    exported = (root / 'etc/openvpn/server1/client.ovpn').read_text()
    assert 'remote 10.250.0.2 1194\n' in exported, exported
    assert 'dev tun\n' in exported
    # Keep all connection/crypto options and inline keys from the actual export.
    # Only name the fixture interface and bridge the emulator's network boundary.
    config = exported.replace('dev tun\n', 'dev tun11\n')
    config += ('\nverb 3\nifconfig-noexec\nroute-noexec\nscript-security 2\n'
               'up /tmp/export/up\n')
    (directory / 'client.ovpn').write_text(config)
    number = 1 if mode == 'static' else 2
    script = directory / 'up'
    script.write_text('#!/bin/sh\nset -eu\n'
                      '/usr/sbin/ip addr replace 10.9.0.%d/24 dev "$dev"\n' % number
                      + '/usr/sbin/ip link set "$dev" up\n'
                        ': > /tmp/export/ready\n')
    script.chmod(0o755)
    logfile = root / 'tmp/exported-client.log'
    logs.append(logfile)
    with logfile.open('w') as output:
        process = subprocess.Popen(['chroot', str(root), '/qemu', '/usr/sbin/openvpn',
            '--cd', '/tmp/export', '--config', 'client.ovpn'], stdout=output, stderr=subprocess.STDOUT)
    processes.append(process)
    await_interface('tun11')
    for attempt in range(100):
        if (directory / 'ready').exists():
            break
        time.sleep(0.05)
    else:
        raise AssertionError('Exported client did not complete its interface setup')
    # Route fixture traffic explicitly; this test verifies exported connection
    # and key material, not the desktop client's route/DNS integration.
    ip('route', 'replace', 'default', 'dev', 'tun11', 'table', 'ovpnc1')
    ip('rule', 'add', 'pref', '1', 'from', '192.0.2.10', 'to', '203.0.113.99',
       'iif', 'br0', 'table', 'ovpnc1')
    transfer(peer, b'RTBE90U-EXPORTED-PROFILE')
    print('PASS actual server-exported ' + mode + ' profile establishes encrypted forwarding', flush=True)
    stop(process)


def main(root, mode):
    network.prepare(root)
    network.setup_links()
    ip('link', 'del', 'tun11')
    (root / 'dev/net').mkdir(parents=True, exist_ok=True)
    for name, major, minor in [('net/tun', 10, 200), ('urandom', 1, 9), ('null', 1, 3)]:
        path = root / 'dev' / name
        path.unlink(missing_ok=True)
        os.mknod(path, stat.S_IFCHR | 0o600, os.makedev(major, minor))
    version = run('chroot', str(root), '/qemu', '/usr/sbin/openvpn', '--version', check=False)
    assert version.stdout.startswith('OpenVPN '), version
    print(version.stdout.splitlines()[0], flush=True)
    run('chroot', str(root), '/qemu', '/usr/sbin/openvpn', '--genkey', '--secret', '/tmp/static.key')
    generate_config(root, mode)
    (root / 'jffs/openvpn/vpndirector_rulelist').write_text('<1>encrypted>192.0.2.10>>OVPN1')
    daemon_env = dict(os.environ, PATH='/usr/sbin:/usr/bin:/sbin:/bin',
                      lan_ifname='br0', vpn_clientx_eas='1,', vpn_client1_rgw='2',
                      vpn_client1_enforce='1', vpn_client1_verb='3', vpn_client1_adns='0',
                      vpn_client1_state='2', jffs2_scripts='0')
    processes = []
    logs = []
    try:
        holder = subprocess.Popen(['unshare', '-n', 'sleep', '300'])
        processes.append(holder)
        for attempt in range(100):
            if os.readlink(f'/proc/{holder.pid}/ns/net') != os.readlink('/proc/self/ns/net'):
                break
            time.sleep(0.01)
        else:
            raise RuntimeError('Could not isolate peer namespace')
        peer = ('nsenter', '-t', str(holder.pid), '-n')
        ip('link', 'add', 'uplink', 'type', 'veth', 'peer', 'name', 'peer0')
        ip('link', 'set', 'peer0', 'netns', str(holder.pid))
        ip('addr', 'add', '10.250.0.1/24', 'dev', 'uplink')
        ip('link', 'set', 'uplink', 'up')
        for args in [('link', 'set', 'lo', 'up'), ('addr', 'add', '10.250.0.2/24', 'dev', 'peer0'),
                     ('link', 'set', 'peer0', 'up'), ('addr', 'add', '203.0.113.99/32', 'dev', 'lo')]:
            run(*peer, 'ip', *args)

        def launch(side, rejected=False):
            iface = 'tun11' if side == 'router' else 'tunpeer'
            prefix = () if side == 'router' else peer
            logfile = root / 'tmp' / (side + '-' + str(len(logs)) + '.log')
            logs.append(logfile)
            marker = root / ('tmp/hook-up-' + ('client' if side == 'router' else 'server'))
            marker.unlink(missing_ok=True)
            with logfile.open('w') as output:
                directory = '/etc/openvpn/' + ('client1' if side == 'router' else 'server1')
                process = subprocess.Popen([*prefix, 'chroot', str(root), '/qemu', '/usr/sbin/openvpn',
                    '--cd', directory, '--config', 'fixture.ovpn'], stdout=output, stderr=subprocess.STDOUT, env=daemon_env)
            processes.append(process)
            if rejected:
                for attempt in range(100):
                    if 'VERIFY X509NAME ERROR' in logfile.read_text():
                        break
                    time.sleep(0.05)
                else:
                    raise AssertionError('Expected server identity rejection was not observed')
                assert ip('link', 'show', iface, check=False).returncode != 0
                return process
            await_interface(iface, prefix)
            for attempt in range(100):
                if marker.exists():
                    break
                time.sleep(0.05)
            else:
                raise AssertionError('Library up hook did not complete')
            return process

        launch('peer')
        client = launch('router')
        client_address = '10.9.0.2' if mode != 'static' else '10.9.0.1'
        run(*peer, 'ip', 'route', 'add', '192.0.2.0/24', 'via', client_address, 'dev', 'tunpeer')
        fixture = network.Fixture(root)
        fixture.rules('<1>encrypted>192.0.2.10>>OVPN1')
        fixture.call('kill', 'ovpn')
        fixture.route('192.0.2.10', '203.0.113.99', 'tun11')
        transfer(peer, b'RTBE90U-ENCRYPTED-INITIAL')
        print('PASS actual ARM OpenVPN ' + mode + ' tunnel with generated configuration forwards LAN packets', flush=True)
        stop(client)
        assert (root / 'tmp/hook-down-client').exists(), 'Library down hook was not called'
        fixture.sockets.pop('tun11').close()
        fixture.route('192.0.2.10', '203.0.113.99', None)
        fixture.packet('192.0.2.10', '203.0.113.99', None)
        print('PASS stopping the actual OpenVPN daemon removes its route and prevents WAN escape', flush=True)
        if mode != 'static':
            config = root / 'etc/openvpn/client1/fixture.ovpn'
            original = config.read_text()
            assert 'verify-x509-name "rtbe90u-test-server"' in original
            config.write_text(original.replace('verify-x509-name "rtbe90u-test-server"',
                                               'verify-x509-name "wrong-fixture-server"'))
            rejected = launch('router', rejected=True)
            fixture.packet('192.0.2.10', '203.0.113.99', None)
            stop(rejected)
            config.write_text(original)
            print('PASS TLS rejects the wrong server identity while the kill switch remains effective', flush=True)
        client = launch('router')
        transfer(peer, b'RTBE90U-ENCRYPTED-RESTART')
        print('PASS actual OpenVPN restart restores encrypted forwarding', flush=True)
        for sock in fixture.sockets.values():
            sock.close()
        stop(client)
        exported_tunnel(root, mode, peer, processes, logs)
    finally:
        for process in reversed(processes):
            stop(process)
        for logfile in logs:
            print(logfile.name + ':\n' + logfile.read_text(), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('root', type=Path)
    parser.add_argument('--mode', choices=('static', 'tls', 'tls-crypt', 'tls-crypt-v2'), default='tls')
    args = parser.parse_args()
    main(args.root, args.mode)
