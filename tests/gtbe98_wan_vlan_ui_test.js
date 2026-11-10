/* SPDX-License-Identifier: GPL-2.0-or-later */
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const www = path.join(__dirname, '../release/src/router/www');
const source = fs.readFileSync(path.join(www, 'Advanced_WAN_Content.asp'), 'utf8');
const begin = source.indexOf('function gtbe98_vlan_validate()');
const end = source.indexOf('\nif(wan_bonding_support)', begin);
assert(begin > 0 && end > begin);
const validator = source.substring(begin, end);
function context() {
    const state = { autowan_enable: '0', wans_dualwan: 'wan none', wans_standby: '0', wans_usb_bk: '0', wans_usb_bk_act: '0', bond_wan: '0', vlan_enable: '0', vlan_rl: '' };
    const result = { gtbe98_vlan_model: true, gtbe98_vlan_cap: true, original_switch_wantag: 'none', original_switch_stb_x: '0', reboot_confirm: 0,
        gtbe98_vlan_original: { enable: '0', vid: '835', priority: '0' },
        document: { form: { wan_dot1q: { value: '1' }, wan_vid: { value: '835' }, wan_dot1p: { value: '0' } } },
        httpApi: { nvramGet: () => state }, messages: [], state };
    result.alert = message => result.messages.push(message);
    vm.createContext(result);
    vm.runInContext(validator, result);
    return result;
}
let c = context();
assert(c.gtbe98_vlan_validate()); assert.equal(c.reboot_confirm, 1);
for (const vid of ['0', '1', '2', '4094', '4095', '-1', '835x', '835 ', '']) {
    c = context(); c.document.form.wan_vid.value = vid; assert(!c.gtbe98_vlan_validate());
}
for (const vid of ['3', '835', '4093']) {
    c = context(); c.document.form.wan_vid.value = vid; assert(c.gtbe98_vlan_validate());
}
for (const priority of ['8', '-1', '00', '0 ', '']) {
    c = context(); c.document.form.wan_dot1p.value = priority; assert(!c.gtbe98_vlan_validate());
}
c = context(); c.original_switch_wantag = 'manual'; assert(!c.gtbe98_vlan_validate());
assert.equal(c.original_switch_wantag, 'manual'); assert(c.messages[0].includes('not migrated automatically'));
for (const [key, value] of Object.entries({ autowan_enable: '1', wans_dualwan: 'wan usb', wans_standby: '1', wans_usb_bk: '1', wans_usb_bk_act: '1', bond_wan: '1', vlan_enable: '1', vlan_rl: '<1>10' })) {
    c = context(); c.state[key] = value; assert(!c.gtbe98_vlan_validate());
}
c = context(); c.gtbe98_vlan_cap = false; assert(!c.gtbe98_vlan_validate()); assert.equal(c.reboot_confirm, 0);
c = context(); c.gtbe98_vlan_cap = false; c.gtbe98_vlan_original.enable = '1'; c.document.form.wan_dot1q.value = '0';
assert(c.gtbe98_vlan_validate()); assert.equal(c.reboot_confirm, 1);
c = context(); c.gtbe98_vlan_original.enable = '1'; c.document.form.wan_dot1q.value = '0'; assert(c.gtbe98_vlan_validate()); assert.equal(c.reboot_confirm, 1);
// Compile the actual page scripts after replacing server template expressions.
for (const file of ['Advanced_WAN_Content.asp', 'Advanced_WANPort_Content.asp', 'Advanced_IPTV_Content.asp']) {
    const html = fs.readFileSync(path.join(www, file), 'utf8');
    for (const match of html.matchAll(/<script\b[^>]*>([\s\S]*?)<\/script>/gi)) {
        const script = match[1].replace(/<%[\s\S]*?%>/g, '0').replace(/<#[\s\S]*?#>/g, 'translated');
        new vm.Script(script, { filename: file });
    }
}
console.log('GT-BE98 WAN VLAN UI validation and page script syntax passed');

// A missing capability must block enable but still allow disabling a saved
// configuration. Execute the actual initialization branch and inspect radios.
const initBegin = source.indexOf('if (gtbe98_vlan_model) {');
const initEnd = source.indexOf('}else if(productid == "BRT-AC828"', initBegin);
for (const enabled of ['0', '1']) {
    c = context(); c.gtbe98_vlan_cap = false; c.gtbe98_vlan_original.enable = enabled;
    c.document.form.wan_dot1q[0] = {}; c.document.form.wan_dot1q[1] = {};
    c.document.getElementById = () => ({});
    c.visibility = {}; c.showhide = (name, visible) => c.visibility[name] = visible;
    vm.runInContext(source.substring(initBegin, initEnd) + '}', c);
    assert.equal(c.visibility.wan_dot1q_setting, enabled == '1' ? 1 : 0);
    assert.equal(c.document.form.wan_dot1q[0].disabled, true);
    assert.equal(c.document.form.wan_dot1q[1].disabled, enabled != '1');
    assert.equal(c.document.form.wan_dot1q.value, enabled);
}

// Exercise the complete existing WAN applyRule and its final FormActions call.
// Browser-independent form mocks distinguish this from a browser acceptance run.
const applyBegin = source.indexOf('function applyRule(){');
const applyEnd = source.indexOf('\nvar ctf =', applyBegin);
const applySource = source.substring(applyBegin, applyEnd).replace(/<%[\s\S]*?%>/g, '0').replace(/<#[\s\S]*?#>/g, 'translated');
const stateSource = fs.readFileSync(path.join(www, 'state.js'), 'utf8');
const actionsBegin = stateSource.indexOf('function FormActions(');
const actionsEnd = stateSource.indexOf('\nfunction change_wl_unit', actionsBegin);
function applyContext() {
    const ctx = context();
    const form = ctx.document.form;
    const radio = () => [{ checked: true, disabled: false }, { checked: false, disabled: false }];
    Object.assign(form, { ewan_dot1q: radio(), ewan_vid: {}, ewan_dot1p: {}, wan_dhcpenable_x: radio(), wan_proto: { value: 'pppoe' },
        dnssec_check_unsigned_x: { value: '0' }, dns_norebind: { value: '0' }, dns_fwd_local: { value: '0' }, dns_priv_override: { value: '0' },
        action_mode: { value: 'apply' }, action_script: { value: 'restart_wan_if' }, action_wait: { value: '5' }, wan_unit: { value: '0' } });
    form.wan_dot1q[0] = {}; form.wan_dot1q[1] = {};
    form.submit = () => {
        ctx.submits++;
        ctx.posted = Object.fromEntries(Object.entries(form).filter(([, field]) => field && typeof field === 'object' && 'value' in field && !field.disabled && !(field[0]?.disabled && field[1]?.disabled)).map(([key, field]) => [key, field.value]));
    };
    Object.assign(ctx, { productid: 'GT-BE98', ctf: { dhcpToPppoe: () => false }, validForm: () => true,
        inputCtrl: () => {}, Softwire46_support: false, wan_bonding_support: false, dnspriv_support: false, dnssec_support: false,
        getRadioValue: control => control.value, isSupport: () => false, isSwMode: () => false,
        wan_proto_orig: 'pppoe', showLoading: () => {}, confirm: () => true, submits: 0 });
    vm.runInContext(stateSource.substring(actionsBegin, actionsEnd), ctx);
    vm.runInContext(applySource, ctx);
    return ctx;
}
c = applyContext(); c.applyRule();
assert.equal(c.submits, 1); assert.equal(c.posted.action_script, 'reboot');
assert.equal(c.posted.wan_dot1q, '1'); assert.equal(c.posted.wan_vid, '835'); assert.equal(c.posted.wan_dot1p, '0'); assert.equal(c.posted.wan_unit, '0');
c = applyContext(); c.gtbe98_vlan_original.enable = '1'; c.document.form.wan_dot1q.value = '0'; c.applyRule();
assert.equal(c.submits, 1); assert.equal(c.posted.action_script, 'reboot'); assert.equal(c.posted.wan_dot1q, '0');
c = applyContext(); c.confirm = () => false; c.applyRule(); assert.equal(c.submits, 0);
c = applyContext(); c.original_switch_wantag = 'manual'; c.applyRule(); assert.equal(c.submits, 0);
c = applyContext(); c.gtbe98_vlan_original.enable = '1'; c.document.form.wan_vid.value = '836'; c.applyRule(); assert.equal(c.posted.action_script, 'reboot');
c = applyContext(); c.gtbe98_vlan_original.enable = '1'; c.document.form.wan_dot1p.value = '7'; c.applyRule(); assert.equal(c.posted.action_script, 'reboot');
c = applyContext(); c.gtbe98_vlan_cap = false; c.gtbe98_vlan_original.enable = '1'; c.document.form.wan_dot1q.value = '0';
c.document.form.wan_vid.disabled = true; c.document.form.wan_dot1p.disabled = true;
c.applyRule(); assert.equal(c.submits, 1); assert.equal(c.posted.action_script, 'reboot'); assert.equal(c.posted.wan_dot1q, '0');
// Evaluate the real model/unit gate, then submit a secondary WAN form. It
// must carry ordinary WAN settings without unit-zero VLAN fields.
const gate = source.match(/^var gtbe98_vlan_model = .*;$/m)[0];
for (const unit of ['', '0', '1', '10']) {
    c = applyContext(); c.document.form.wan_unit.value = unit;
    vm.runInContext(gate.replace(/<%[\s\S]*?%>/g, unit), c);
    assert.equal(c.gtbe98_vlan_model, unit === '' || unit === '0');
    if (!c.gtbe98_vlan_model) {
        c.applyRule(); assert.equal(c.submits, 1);
        for (const key of ['wan_dot1q', 'wan_vid', 'wan_dot1p']) assert(!(key in c.posted));
        assert.equal(c.posted.wan_proto, 'pppoe');
        assert.equal(c.posted.action_script, 'restart_wan_if');
    }
}
console.log('GT-BE98 complete WAN applyRule form/reboot behavior passed (mock DOM)');
