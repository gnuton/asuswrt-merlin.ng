#!/usr/bin/env python3
"""Test local curl against a loopback-only fixture of ASUS's TFTP wire format.

No router connection, firmware upload to hardware, flash emulation or bootloader
execution. Checks binary transfer and block-counter rollover with an actual image.
"""
import argparse
import hashlib
from pathlib import Path
import socket
import struct
import subprocess
import tempfile
import threading


def exercise(image):
    expected = hashlib.sha256(image.read_bytes()).hexdigest()
    size = image.stat().st_size
    assert size > 65535 * 512, 'Use a full image to exercise block-counter rollover'
    failures = []
    result = {}
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server, tempfile.TemporaryDirectory() as temp:
        server.bind(('127.0.0.1', 0))
        server.settimeout(30)

        def serve():
            try:
                packet, peer = server.recvfrom(2048)
                assert packet == b'\0\1rt-be90u-probe\0octet\0', packet
                server.sendto(struct.pack('!HH', 3, 1), peer)
                packet, peer = server.recvfrom(2048)
                assert packet == struct.pack('!HH', 4, 1), packet
                packet, peer = server.recvfrom(2048)
                assert packet == b'\0\2TUF-BE9400.trx\0octet\0', packet
                server.sendto(struct.pack('!HH', 4, 0), peer)
                digest = hashlib.sha256()
                count = 0
                blocks = 0
                while True:
                    packet, source = server.recvfrom(2048)
                    blocks += 1
                    assert source == peer
                    assert len(packet) <= 516
                    assert struct.unpack('!HH', packet[:4]) == (3, blocks & 65535)
                    digest.update(packet[4:])
                    count += len(packet) - 4
                    server.sendto(struct.pack('!HH', 4, blocks & 65535), peer)
                    if len(packet) < 516:
                        break
                assert count == size and digest.hexdigest() == expected
                result.update(bytes=count, blocks=blocks, sha256=expected)
            except Exception as error:
                failures.append(error)

        thread = threading.Thread(target=serve, daemon=True)
        thread.start()
        url = 'tftp://127.0.0.1:%d/' % server.getsockname()[1]
        options = ['curl', '--silent', '--show-error', '--noproxy', '*',
                   '--proto', '=tftp', '--tftp-no-options', '--max-time', '60']
        probe = Path(temp) / 'probe'
        subprocess.run(options + ['--output', str(probe), url + 'rt-be90u-probe'], check=True, timeout=65)
        assert probe.read_bytes() == b''
        subprocess.run(options + ['--upload-file', str(image), url + 'TUF-BE9400.trx'], check=True, timeout=65)
        thread.join(timeout=35)
        assert not thread.is_alive()
        if failures:
            raise failures[0]
    print('PASS curl empty-read probe, option-free 512-byte binary upload and block-counter rollover:', result)
    print('Fixture only: installed bootloader acceptance and flash completion remain unverified.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    exercise(parser.parse_args().image)
