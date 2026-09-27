const assert = require('node:assert/strict');
const { chromium } = require('playwright');
const { execFileSync } = require('node:child_process');
const { mkdtempSync, readFileSync, rmSync } = require('node:fs');
const { createServer } = require('node:http');
const { tmpdir } = require('node:os');
const { join } = require('node:path');

const baseURL = process.env.RTBE90U_TEST_URL || 'http://127.0.0.1:18090';
assert.equal(new URL(baseURL).origin, baseURL, 'Use an origin without a path');
assert.equal(new URL(baseURL).hostname, '127.0.0.1', 'Use the isolated localhost fixture');
const paths = [
  'Advanced_VPNDirector.asp', 'Advanced_OpenVPNClient_Content.asp',
  'Advanced_VPN_OpenVPN.asp', 'Advanced_WireguardClient_Content.asp',
  'Advanced_WireguardServer_Content.asp', 'Advanced_VPNStatus.asp'
];

(async () => {
  const certificates = mkdtempSync(join(tmpdir(), 'rtbe90u-certificates-'));
  const blockedProxy = createServer(request => request.destroy());
  blockedProxy.on('connect', (_request, socket) => socket.destroy());
  let browser;
  try {
    await new Promise((resolve, reject) => {
      blockedProxy.once('error', reject);
      blockedProxy.listen(0, '127.0.0.1', resolve);
    });
    execFileSync('openssl', ['req', '-x509', '-newkey', 'rsa:2048', '-nodes',
      '-keyout', join(certificates, 'key.pem'), '-out', join(certificates, 'cert.pem'),
      '-days', '1', '-subj', '/CN=rtbe90u-fixture.invalid'], { stdio: 'ignore' });
    const certificate = readFileSync(join(certificates, 'cert.pem'), 'utf8');
    const privateKey = readFileSync(join(certificates, 'key.pem'), 'utf8');
    const chain = certificate.repeat(6);
    assert(chain.length > 4000 && chain.length < 7999);
    // Blocking proxy keeps external traffic offline without intercepting the UI's synchronous XHRs.
    browser = await chromium.launch({ headless: true, proxy: {
      server: `http://127.0.0.1:${blockedProxy.address().port}`, bypass: '127.0.0.1'
    } });
    const context = await browser.newContext({ baseURL });
    context.setDefaultTimeout(10000);
    context.setDefaultNavigationTimeout(30000);
    const marker = await context.request.get('/Main_Login.asp');
    assert((await marker.text()).includes('RTBE90U_HTTPD_FIXTURE'), 'Refusing to modify a non-fixture server');
    for (const unit of ['1', '2']) {
      const denied = await context.request.get(`/client${unit}.ovpn`);
      assert(!(await denied.text()).includes('RTBE90U_EXPORT_FIXTURE_'), 'Unauthenticated export disclosed a profile');
    }
    for (const name of ['wgs_client.conf', 'wgs_client.png']) {
      const denied = await context.request.get('/' + name);
      assert(!(await denied.body()).includes(Buffer.from('RTBE90U_WG_PRIVATE_FIXTURE_')),
        'Unauthenticated WireGuard export disclosed private profile data: ' + name);
    }
    const login = await context.request.post('/login.cgi', {
      headers: { Referer: baseURL + '/Main_Login.asp' },
      form: {
        login_authorization: Buffer.from('admin:rtbe90u-fixture').toString('base64'),
        auth_version: '1', next_page: paths[0]
      }
    });
    assert.equal(login.status(), 200);
    assert((await context.cookies()).some(cookie => cookie.name === 'asus_token'), 'Fixture login failed');

    let page;
    const errors = [];
    let expectedConfirmation = false;
    context.on('page', opened => {
      opened.on('pageerror', error => errors.push(error.stack));
      opened.on('response', response => {
        if (response.status() >= 400) errors.push(`${response.status()} ${response.url()}`);
      });
      opened.on('dialog', async dialog => {
        if (expectedConfirmation && dialog.type() === 'confirm') {
          expectedConfirmation = false;
          await dialog.accept();
        } else {
          errors.push('Unexpected dialog: ' + dialog.message());
          await dialog.dismiss();
        }
      });
    });
    page = await context.newPage();
    const healthy = () => assert.deepEqual(errors, [], 'Browser errors');
    async function ready() {
      await page.waitForFunction(() => document.querySelector('#tabMenu').children.length > 0);
      if ([paths[1], paths[2]].some(path => page.url().endsWith('/' + path))) {
        await page.waitForFunction(() => typeof vpn_crt_client1_ca !== 'undefined');
      }
      healthy();
    }
    async function open(path) {
      await page.goto('/' + path);
      await ready();
      assert.equal(new URL(page.url()).pathname, '/' + path, 'Unexpected setup/login redirect');
      healthy();
    }
    async function reopen() {
      const url = page.url();
      // End the old page's polling and delayed iframe redirects before reading saved values.
      await page.close();
      page = await context.newPage();
      await page.goto(url);
      await ready();
    }
    async function apply() {
      try {
        const [response] = await Promise.all([
          page.waitForResponse(response => response.url().endsWith('/start_apply.htm'), { timeout: 10000 }),
          page.getByRole('button', { name: 'Apply', exact: true }).click()
        ]);
        assert.equal(response.status(), 200);
        await response.finished();
        await response.frame().waitForURL('**/start_apply.htm', { waitUntil: 'load' });
      } finally {
        healthy();
      }
      await reopen();
    }
    const field = name => page.locator(`form[name="form"] [name="${name}"]`);
    async function selectClient(unit) {
      if (await field('vpn_client_unit').inputValue() === unit) return;
      await Promise.all([
        page.waitForNavigation({ waitUntil: 'load' }),
        field('vpn_client_unit').selectOption(unit)
      ]);
      await ready();
      await value('vpn_client_unit', unit);
    }
    async function value(name, expected) {
      assert.equal(await field(name).inputValue(), expected, name + ' did not survive reload');
    }

    for (const path of paths) {
      await open(path);
      for (const tab of paths) {
        assert(await page.locator(`#tabMenu [title="${tab}"]`).isVisible(), 'Missing VPN tab: ' + tab);
      }
      console.log('PASS render and navigation: ' + path);
    }

    await open(paths[0]);
    while (await page.locator('.remove_btn').count()) {
      expectedConfirmation = true;
      await page.locator('.remove_btn').first().click();
    }
    await page.locator('.add_btn').click();
    await page.locator('#desc_x').fill('fixture-route');
    await page.locator('#localIP_x').fill('192.0.2.0/24');
    await page.locator('#remoteIP_x').fill('198.51.100.0/24');
    await page.locator('#iface_x').selectOption('OVPN1');
    await page.locator('#saveRule').click();
    await apply();
    const row = () => page.locator('[row_tr_idx]').filter({ hasText: 'fixture-route' });
    assert.equal(await row().count(), 1);
    assert((await row().innerText()).includes('192.0.2.0/24'));
    await row().locator('img[title="Enabled"]').hover();
    await row().locator('img[title="Enabled"]').click();
    await row().locator('img[title="Disabled"]').hover();
    await row().locator('.edit_btn').click();
    await page.locator('#remoteIP_x').fill('203.0.113.0/24');
    await page.locator('#saveRule').click();
    await apply();
    assert.equal(await row().locator('img[title="Disabled"]').count(), 1);
    assert((await row().innerText()).includes('203.0.113.0/24'));
    expectedConfirmation = true;
    await row().locator('.remove_btn').click();
    await apply();
    assert.equal(await page.locator('[row_tr_idx]').count(), 0);
    console.log('PASS VPN Director create, disable, edit, delete and reload');

    const password = 'p'.repeat(128);
    const custom = 'setenv TEST "quoted & value"\n# fixture custom configuration\n';
    await open(paths[1]);
    await selectClient('1');
    await field('vpn_client_addr').fill('vpn.example.test');
    await field('vpn_client_desc').fill('Fixture Client 1');
    await page.locator('[name="vpn_client_userauth"][value="1"]').check();
    await field('vpn_client_username').fill('fixture-user');
    await field('vpn_client_password').fill(password);
    await field('vpn_client_custom3').fill(custom);
    await field('vpn_client_ncp_ciphers').fill('AES-256-GCM:AES-128-GCM');
    await apply();
    await value('vpn_client_addr', 'vpn.example.test');
    await value('vpn_client_desc', 'Fixture Client 1');
    await value('vpn_client_password', password);
    await value('vpn_client_custom3', custom);
    await value('vpn_client_ncp_ciphers', 'AES-256-GCM:AES-128-GCM');
    await selectClient('2');
    await field('vpn_client_desc').fill('Fixture Client 2');
    await field('vpn_client_custom3').fill('# second client\n');
    await apply();
    await value('vpn_client_desc', 'Fixture Client 2');
    await value('vpn_client_custom3', '# second client\n');
    await selectClient('1');
    await value('vpn_client_desc', 'Fixture Client 1');
    await value('vpn_client_password', password);
    await value('vpn_client_custom3', custom);
    await field('vpn_client_custom3').fill('');
    await apply();
    await value('vpn_client_custom3', '');
    console.log('PASS OpenVPN client credentials, ciphers, profile isolation and custom save/clear');
    await page.locator('[onclick="edit_Keys();"]').click();
    await page.locator('#edit_vpn_crt_client_ca').fill(certificate);
    await page.locator('#edit_vpn_crt_client_key').fill(privateKey);
    await page.locator('#edit_vpn_crt_client_extra').fill(chain);
    await page.locator('[onclick="save_Keys();"]').click();
    await apply();
    await page.locator('[onclick="edit_Keys();"]').click();
    assert.equal(await page.locator('#edit_vpn_crt_client_ca').inputValue(), certificate);
    assert.equal(await page.locator('#edit_vpn_crt_client_key').inputValue(), privateKey);
    assert.equal(await page.locator('#edit_vpn_crt_client_extra').inputValue(), chain);
    await page.locator('[onclick="cancel_Keys();"]').click();
    await selectClient('2');
    const profile = 'client\ndev tun\nproto udp\nremote import.example.test 1443\nremote-cert-tls server\n' +
      '<ca>\n' + certificate + '</ca>\n<cert>\n' + certificate + '</cert>\n<key>\n' + privateKey +
      '</key>\n<extra-certs>\n' + chain + '</extra-certs>\n';
    await page.locator('#ovpnfile').setInputFiles({
      name: 'fixture.ovpn', mimeType: 'application/octet-stream', buffer: Buffer.from(profile)
    });
    await Promise.all([
      page.waitForNavigation({ waitUntil: 'load' }),
      page.locator('[onclick="ImportOvpn();"]').click()
    ]);
    await ready();
    await value('vpn_client_unit', '2');
    await value('vpn_client_addr', 'import.example.test');
    await value('vpn_client_port', '1443');
    assert.equal(await page.locator('#edit_vpn_crt_client_ca').inputValue(), certificate);
    assert.equal(await page.locator('#edit_vpn_crt_client_key').inputValue(), privateKey);
    assert.equal(await page.locator('#edit_vpn_crt_client_extra').inputValue(), chain);

    async function postKeys(form) {
      const response = await context.request.post('/start_apply.htm', {
        headers: { Referer: page.url() },
        form: { action_mode: 'apply', action_script: '', action_wait: '0', ...form }
      });
      assert.equal(response.status(), 200);
      await reopen();
    }
    await postKeys({ vpn_client_unit: '2', vpn_crt_client_ca: chain });
    assert.equal(await page.locator('#edit_vpn_crt_client_ca').inputValue(), chain);
    await postKeys({ vpn_client_unit: '2', vpn_crt_client_ca: certificate.repeat(8) });
    assert.equal(await page.locator('#edit_vpn_crt_client_ca').inputValue(), chain, 'Oversized input changed the certificate');
    await postKeys({ vpn_crt_client2_ca: certificate.repeat(8) });
    assert.equal(await page.locator('#edit_vpn_crt_client_ca').inputValue(), chain, 'Oversized indexed input changed the certificate');
    await postKeys({ vpn_client_unit: '2', vpn_crt_client_ca: '', vpn_crt_client_extra: '' });
    assert.equal(await page.locator('#edit_vpn_crt_client_ca').inputValue(), '');
    assert.equal(await page.locator('#edit_vpn_crt_client_extra').inputValue(), '');
    await selectClient('1');
    assert.equal(await page.locator('#edit_vpn_crt_client_ca').inputValue(), certificate);
    assert.equal(await page.locator('#edit_vpn_crt_client_extra').inputValue(), chain);
    await page.locator('[onclick="edit_Keys();"]').click();
    await page.locator('#edit_vpn_crt_client_extra').fill('');
    await page.locator('[onclick="save_Keys();"]').click();
    await apply();
    assert.equal(await page.locator('#edit_vpn_crt_client_extra').inputValue(), '');
    console.log('PASS OpenVPN certificate editing, long chains, import, clearing and profile isolation');

    await open(paths[2]);
    if (!(await page.locator('#selSwitchMode').isVisible())) await page.locator('#radio_VPNServer_enable').click();
    await page.locator('#selSwitchMode').selectOption('2');
    await field('vpn_server_port').fill('1195');
    await field('vpn_server_custom3').fill(custom);
    await apply();
    await value('vpn_server_port', '1195');
    await value('vpn_server_custom3', custom);
    if (!(await page.locator('#selSwitchMode').isVisible())) await page.locator('#radio_VPNServer_enable').click();
    await page.locator('#selSwitchMode').selectOption('2');
    await field('vpn_server_custom3').fill('');
    await apply();
    await value('vpn_server_custom3', '');
    console.log('PASS OpenVPN server port and custom save/clear');
    if (!(await page.locator('#selSwitchMode').isVisible())) await page.locator('#radio_VPNServer_enable').click();
    await page.locator('#selSwitchMode').selectOption('2');
    await page.locator('[onclick="edit_Keys();"]').click();
    await page.locator('#edit_vpn_crt_server_ca').fill(certificate);
    await page.locator('#edit_vpn_crt_server_key').fill(privateKey);
    await page.locator('#edit_vpn_crt_server_extra').fill(chain);
    const savedKeys = page.waitForResponse(response => response.url().endsWith('/start_apply.htm'));
    await page.locator('[onclick="save_keys();"]').click();
    const keysResponse = await savedKeys;
    assert.equal(keysResponse.status(), 200);
    await keysResponse.finished();
    await keysResponse.frame().waitForURL('**/start_apply.htm', { waitUntil: 'load' });
    await reopen();
    assert.equal(await page.locator('#edit_vpn_crt_server_ca').inputValue(), certificate);
    assert.equal(await page.locator('#edit_vpn_crt_server_key').inputValue(), privateKey);
    assert.equal(await page.locator('#edit_vpn_crt_server_extra').inputValue(), chain);
    await postKeys({ vpn_server_unit: '1', vpn_crt_server_ca: chain, vpn_crt_server_extra: '' });
    assert.equal(await page.locator('#edit_vpn_crt_server_ca').inputValue(), chain);
    assert.equal(await page.locator('#edit_vpn_crt_server_extra').inputValue(), '');
    console.log('PASS OpenVPN server certificate editing and clearing');

    for (const unit of ['1', '2']) {
      if (await field('vpn_server_unit').inputValue() !== unit) {
        await Promise.all([
          page.waitForNavigation({ waitUntil: 'load' }),
          field('vpn_server_unit').selectOption(unit)
        ]);
        await ready();
      }
      const downloaded = page.waitForEvent('download');
      // Fixture files stand in for rc-generated profiles; rc is not running.
      // Invoke the actual export handler even when its daemon-state UI is hidden.
      await page.locator('#exportToLocal').evaluate(button => button.click());
      const download = await downloaded;
      assert.equal(download.suggestedFilename(), `client${unit}.ovpn`);
      assert.equal(readFileSync(await download.path(), 'utf8'),
        `# RTBE90U_EXPORT_FIXTURE_${unit}\nclient\nremote 192.0.2.${unit} 1194\n`);
      healthy();
    }
    console.log('PASS OpenVPN server 1/2 export buttons, exact profile selection and unauthenticated blocking');

    await open(paths[3]);
    await page.locator('[name="wgc_enable"][value="0"]').check();
    await field('wgc_desc').fill('Fixture WG client');
    await page.locator('[name="wgc_enforce"][value="1"]').check();
    await apply();
    await value('wgc_desc', 'Fixture WG client');
    assert(await page.locator('[name="wgc_enforce"][value="1"]').isChecked());
    console.log('PASS WireGuard client description and enforcement setting');

    async function selectWg(name, unit) {
      if (await field(name).inputValue() === unit) return;
      await Promise.all([
        page.waitForNavigation({ waitUntil: 'load' }),
        field(name).selectOption(unit)
      ]);
      await ready();
      await value(name, unit);
    }
    await selectWg('wgc_unit', '2');
    const wgPrivate = Buffer.alloc(32, 7).toString('base64');
    const wgPublic = Buffer.alloc(32, 9).toString('base64');
    const wgPsk = Buffer.alloc(32, 11).toString('base64');
    const wgConfig = `[Interface]\nPrivateKey = ${wgPrivate}\nAddress = 10.77.2.2/32\n` +
      `DNS = 10.77.2.1\nMTU = 1380\n[Peer]\nPublicKey = ${wgPublic}\nPresharedKey = ${wgPsk}\n` +
      'AllowedIPs = 0.0.0.0/0,::/0\nEndpoint = [2001:db8::77]:51822\nPersistentKeepalive = 25\n';
    await page.locator('#wgfile').setInputFiles({
      name: 'fixture.conf', mimeType: 'text/plain', buffer: Buffer.from(wgConfig)
    });
    const imported = page.waitForResponse(response => response.url().endsWith('/upload_wgc_config.cgi'));
    await page.locator('[onclick="Importwg();"]').click();
    const importedResponse = await imported;
    assert.equal(importedResponse.status(), 200);
    await importedResponse.finished();
    await reopen();
    for (const [name, expected] of Object.entries({
      wgc_priv: wgPrivate, wgc_ppub: wgPublic, wgc_psk: wgPsk,
      wgc_addr: '10.77.2.2/32', wgc_dns: '10.77.2.1', wgc_mtu: '1380',
      wgc_aips: '0.0.0.0/0,::/0', wgc_ep_addr: '2001:db8::77', wgc_ep_port: '51822', wgc_alive: '25'
    })) await value(name, expected);
    for (const [unit, config] of [
      ['2', ''], ['2', '[Interface]\nPrivateKey\n'],
      ['2', wgConfig + '\n[Peer]\nPublicKey = ' + wgPublic + '\n'],
      ['2junk', wgConfig], ['6', wgConfig], ['', wgConfig]
    ]) {
      const rejected = await context.request.post('/upload_wgc_config.cgi', {
        headers: { Referer: page.url() },
        multipart: {
          wgc_upload_unit: unit,
          file: { name: 'rejected.conf', mimeType: 'text/plain', buffer: Buffer.from(config) }
        }
      });
      assert.equal(rejected.status(), 200);
      await reopen();
      assert.equal(await page.evaluate(() => httpApi.nvramGet(['wgc_upload_state'], true).wgc_upload_state), 'err');
      await value('wgc_priv', wgPrivate);
      await value('wgc_ppub', wgPublic);
      await value('wgc_addr', '10.77.2.2/32');
      await value('wgc_aips', '0.0.0.0/0,::/0');
    }
    console.log('PASS rejected WireGuard imports preserve settings and HTTP remains available');
    await selectWg('wgc_unit', '1');
    await value('wgc_desc', 'Fixture WG client');
    assert.notEqual(await field('wgc_priv').inputValue(), wgPrivate, 'Import changed another client profile');
    console.log('PASS WireGuard profile import, IPv6 endpoint, keys and client-unit isolation');

    await open(paths[4]);
    await field('wgs_port').fill('51821');
    await apply();
    await value('wgs_port', '51821');
    console.log('PASS WireGuard server port');
    for (const unit of ['1', '2']) {
      await selectWg('wgsc_unit', unit);
      await page.locator('[name="wgsc_enable"][value="1"]').check();
      for (const [name, value] of Object.entries({
        wgsc_name: `Fixture peer ${unit}`, wgsc_addr: `10.88.0.${unit}/32`,
        wgsc_aips: `10.88.0.${unit}/32`, wgsc_caips: unit === '1' ? '0.0.0.0/0' : '192.0.2.0/24'
      })) await field(name).fill(value);
      await apply();
    }
    await selectWg('wgsc_unit', '1');
    await value('wgsc_name', 'Fixture peer 1');
    await value('wgsc_addr', '10.88.0.1/32');
    await value('wgsc_caips', '0.0.0.0/0');
    await page.locator('[name="wgsc_enable"][value="0"]').check();
    await apply();
    assert(await page.locator('[name="wgsc_enable"][value="0"]').isChecked());
    await selectWg('wgsc_unit', '2');
    await value('wgsc_name', 'Fixture peer 2');
    await value('wgsc_addr', '10.88.0.2/32');
    await value('wgsc_caips', '192.0.2.0/24');
    assert(await page.locator('[name="wgsc_enable"][value="1"]').isChecked());
    console.log('PASS WireGuard peer editing, disable and server-peer isolation');
    for (const unit of ['1', '2']) {
      await selectWg('wgsc_unit', unit);
      const downloaded = page.waitForEvent('download');
      await page.locator('[onclick="exportConfig();"]').evaluate(button => button.click());
      const download = await downloaded;
      assert.equal(download.suggestedFilename(), 'wgs_client.conf');
      assert.equal(readFileSync(await download.path(), 'utf8'), `RTBE90U_WG_PRIVATE_FIXTURE_${unit}\n`);
      const qr = await context.request.get('/wgs_client.png');
      assert.equal(qr.status(), 200);
      assert((await qr.body()).includes(Buffer.from(`RTBE90U_WG_PRIVATE_FIXTURE_${unit}`)));
    }
    console.log('PASS authenticated WireGuard configuration/QR exports select the correct peer');
    healthy();
  } finally {
    if (browser) await browser.close();
    blockedProxy.close();
    rmSync(certificates, { recursive: true, force: true });
  }
})().catch(error => { console.error(error); process.exitCode = 1; });
