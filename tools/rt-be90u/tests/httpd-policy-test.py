#!/usr/bin/env python3
"""Exercise policy-only Apply through the unmodified packaged HTTP daemon."""
import base64
from contextlib import contextmanager
import http.cookiejar
import os
from pathlib import Path
import re
import signal
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request


def main():
    root = Path(sys.argv[1])
    # This test never starts a server on a host or a routed container network.
    assert Path('/.dockerenv').exists()
    assert set(os.listdir('/sys/class/net')) == {'lo'}, 'Use --network none'
    for name in ('random', 'urandom'):
        (root / 'dev' / name).write_bytes(os.urandom(1024 * 1024))
    (root / 'proc/uptime').write_text(Path('/proc/uptime').read_text())
    policy = root / 'jffs/openvpn/vpndirector_rulelist'
    durable = root / 'tmp/policy-durable'
    commits = root / 'tmp/policy-commits'
    old_rules = '<1>Migrated policy>192.0.2.10>>OVPN1'
    new_rules = '<1>Edited policy>192.0.2.20>>WGC1'
    durable.write_text(old_rules)
    commits.write_text('')
    settings = {
        'productid': 'TUF-BE9400', 'odmpid': 'RT-BE90U',
        'firmver': '3.0.0.6', 'buildno': '102', 'extendno': 'rtbe90u-http-policy-test',
        'sw_mode': '1', 'x_Setting': '1', 'p_Setting': '1', 'w_Setting': '1',
        'ASUS_NEW_EULA': '1', 'ASUS_NEW_EULA_time': '2026-09-25T00:00:00Z',
        '68feaaa3': '2', '767cd197': '2026-09-25T00:00:00Z', 'TM_EULA': '0',
        'sdn_rl': '<0>TEST>1>0>0>0>0>0>0>0>0>0>0>0>0>0>0>0>0>WEB>0>0>0',
        'http_username': 'admin', 'http_passwd': 'rtbe90u-fixture',
        'lan_ifname': 'lo', 'lan_ipaddr': '127.0.0.1', 'lan_netmask': '255.0.0.0',
        'http_enable': '0', 'captcha_enable': '0',
        'wl_ifnames': 'ath0 ath1 ath2', 'wl0_nband': '2', 'wl1_nband': '1', 'wl2_nband': '4',
        'rc_support': '2.4G 5G 6G openvpnd vpnc vpn_fusion wireguard mtlancfg odmpid ipv6',
        'vpn_client_unit': '1', 'vpn_server_unit': '1', 'wgc_unit': '1', 'wgs_unit': '1',
        'qca_merlin_vpn_migrated': '1', 'vpn_clientx_eas': '',
    }

    @contextmanager
    def server():
        with socket.socket() as reservation:
            reservation.bind(('127.0.0.1', 0))
            port = reservation.getsockname()[1]
        origin = 'http://127.0.0.1:%d' % port
        env = os.environ.copy()
        values = dict(settings, http_lanport=str(port), vpndirector_rulelist=durable.read_text())
        env.update({'RTBE90U_NVRAM_' + name: value for name, value in values.items()})
        cookies = http.cookiejar.CookieJar()
        client = urllib.request.build_opener(urllib.request.ProxyHandler({}),
                                            urllib.request.HTTPCookieProcessor(cookies))

        def request(path, values=None, referer='/Advanced_VPNDirector.asp'):
            data = None if values is None else urllib.parse.urlencode(values).encode()
            req = urllib.request.Request(origin + path, data=data,
                                         headers={'Referer': origin + referer})
            with client.open(req, timeout=15) as response:
                assert response.status == 200
                assert response.geturl().startswith(origin + '/'), response.geturl()
                return response.read().decode()

        def enter_root():
            os.chroot(root)
            os.chdir('/www')

        with (root / 'tmp/policy-httpd.log').open('w') as log:
            process = subprocess.Popen(['/qemu', '-E', 'LD_PRELOAD=/tmp/httpd-policy-nvram.so',
                                        '/usr/sbin/httpd', '-i', 'lo', '-p', str(port)],
                                       env=env, preexec_fn=enter_root, start_new_session=True,
                                       stdout=log, stderr=log)
            try:
                deadline = time.monotonic() + 20
                while True:
                    assert process.poll() is None, (root / 'tmp/policy-httpd.log').read_text()
                    try:
                        request('/Main_Login.asp', referer='/Main_Login.asp')
                        break
                    except (OSError, urllib.error.URLError):
                        assert time.monotonic() < deadline, (root / 'tmp/policy-httpd.log').read_text()
                        time.sleep(0.1)
                request('/login.cgi', {
                    'login_authorization': base64.b64encode(b'admin:rtbe90u-fixture').decode(),
                    'auth_version': '1', 'next_page': 'Advanced_VPNDirector.asp',
                }, referer='/Main_Login.asp')
                assert any(cookie.name == 'asus_token' for cookie in cookies), 'Fixture login failed'
                yield request
            finally:
                if process.poll() is None:
                    os.killpg(process.pid, signal.SIGTERM)
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid, signal.SIGKILL)
                        process.wait(timeout=5)

    def apply(request, value):
        return request('/start_apply.htm', {
            'action_mode': 'apply', 'action_script': 'restart_vpnrouting0', 'action_wait': '1',
            'current_page': 'Advanced_VPNDirector.asp', 'next_page': 'Advanced_VPNDirector.asp',
            'vpndirector_rulelist': value, 'vpn_clientx_eas': '',
        })

    with server() as request:
        before = commits.read_text()
        # An unwritable destination must leave the durable fallback untouched.
        policy.mkdir()
        response = apply(request, new_rules)
        assert '<script>no_changes_and_no_committing();</script>' in response, response[-512:]
        assert commits.read_text() == before and durable.read_text() == old_rules
        policy.rmdir()
        print('PASS real HTTP failed policy Apply does not commit fallback removal', flush=True)

        response = apply(request, new_rules)
        assert '<script>done_committing();</script>' in response, response[-512:]
        assert commits.read_text() == before + 'commit\n', (
            'Policy-only Apply did not commit exactly once', before, commits.read_text(),
            durable.read_text(), (root / 'tmp/policy-httpd.log').read_text()[-2000:])
        assert durable.read_text() == '' and policy.read_text() == new_rules
        print('PASS real HTTP policy-only Apply commits fallback removal', flush=True)

        before = commits.read_text()
        response = apply(request, '')
        assert '<script>no_changes_and_no_committing();</script>' in response, response[-512:]
        assert commits.read_text() == before and policy.read_text() == ''
        print('PASS later explicit empty policy save needs no NVRAM commit', flush=True)

    policy.unlink()
    with server() as request:
        # Use the real ASP policy reader after a fresh process loads durable NVRAM.
        page = request('/Advanced_VPNDirector.asp')
        assert re.search(r"vpndirector_rulelist_array\s*=\s*parseNvramToArray\(''\);", page), \
            'Retired migration policy returned after restart'
        assert durable.read_text() == ''
        print('PASS fresh HTTP process cannot resurrect retired rules after policy-file loss', flush=True)

    # Clearing every migrated rule is itself a first save, not just a later
    # edit of an already published policy file.
    durable.write_text(old_rules)
    before = commits.read_text()
    with server() as request:
        response = apply(request, '')
        assert '<script>done_committing();</script>' in response, response[-512:]
        assert commits.read_text() == before + 'commit\n'
        assert durable.read_text() == '' and policy.read_text() == ''
    policy.unlink()
    with server() as request:
        page = request('/Advanced_VPNDirector.asp')
        assert re.search(r"vpndirector_rulelist_array\s*=\s*parseNvramToArray\(''\);", page), \
            'First empty save did not retire migrated rules'
        assert durable.read_text() == ''
    print('PASS first empty policy Apply commits retirement and stays empty after restart', flush=True)
    print('HTTP policy apply fixture passed: 5 scenarios', flush=True)


if __name__ == '__main__':
    main()
