#!/usr/bin/env python3
"""Execute the actual legacy WAN migration functions with an in-memory NVRAM.

Also check that dedicated endpoint guards precede calls which can mutate NVRAM.
This is source/host evidence, not an HTTP server or firmware acceptance test.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[1]
web = (repo / 'release/src/router/httpd/web.c').read_text()
first = web.index('void save_index_to_interface(){')
last = web.index('\n#endif', web.index('void save_interface_to_index(', first))
# Internal GTBE98 guards also contain #endif: find the next top-level function.
last = web.index('\n#ifdef RTCONFIG_JFFS2USERICON', last)
functions = web[first:last].rsplit('\n#endif', 1)[0]
harness = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GTBE98
struct nvram_tuple { const char *name; const char *value; };
struct nvram_tuple router_defaults[] = {
    {"wan_dot1q", "0"}, {"wan_vid", ""}, {"wan_dot1p", "0"},
    {"wan_proto", "dhcp"}, {NULL, NULL}
};
struct item { char *name; char *value; } values[64];
static unsigned int count;
static char *nvram_get(const char *name) {
    unsigned int i;
    for (i=0; i<count; ++i) if (!strcmp(values[i].name, name)) return values[i].value;
    return NULL;
}
static void nvram_set(const char *name, const char *value) {
    unsigned int i;
    for (i=0; i<count; ++i) if (!strcmp(values[i].name, name)) {
        free(values[i].value); values[i].value=strdup(value); return;
    }
    assert(count < 64);
    values[count].name=strdup(name); values[count++].value=strdup(value);
}
static void nvram_unset(const char *name) {
    unsigned int i;
    for (i=0; i<count; ++i) if (!strcmp(values[i].name,name)) {
        free(values[i].value); values[i].value=NULL;
    }
}
static char *strlcat_r(const char *a, const char *b, char *out, size_t size) {
    snprintf(out,size,"%s%s",a,b); return out;
}
#define foreach(word, list, next) \
    for (next=(list); next && *next && sscanf(next, "%79s", word)==1; \
         next=strchr(next, ' '), next=next?next+1:NULL)
'''
main = r'''
int main(void) {
    unsigned int i;
    nvram_set("wans_dualwan", "wan usb");
    nvram_set("wan0_dot1q", "0"); nvram_set("wan0_vid", "835"); nvram_set("wan0_dot1p", "2");
    nvram_set("wan1_dot1q", "1"); nvram_set("wan1_vid", "836"); nvram_set("wan1_dot1p", "3");
    nvram_set("wan0_proto", "pppoe"); nvram_set("wan1_proto", "dhcp");
    nvram_set("usb_wan_dot1q", "1"); nvram_set("usb_wan_vid", "836"); nvram_set("usb_wan_dot1p", "3");
    save_index_to_interface();
    save_interface_to_index("usb wan");
    assert(!strcmp(nvram_get("wan0_dot1q"), "0"));
    assert(!strcmp(nvram_get("wan0_vid"), "835"));
    assert(!strcmp(nvram_get("wan0_dot1p"), "2"));
    assert(!strcmp(nvram_get("wan0_proto"), "dhcp"));
    assert(!strcmp(nvram_get("wan1_proto"), "pppoe"));
    for (i=0; i<count; ++i) { free(values[i].name); free(values[i].value); }
    puts("GT-BE98 actual WAN migration functions preserve canonical VLAN fields");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='gtbe98-http-integration-') as work:
    src = Path(work) / 'test.c'
    exe = Path(work) / 'test'
    src.write_text(harness + functions + main)
    # Existing migration code has conservative GCC truncation warnings for word[80].
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu89', '-Wall', '-Wextra', '-Werror', '-Wno-format-truncation', str(src), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

def function_body(name):
    # Match a definition, never an earlier prototype or a call. These legacy
    # functions end with an unindented brace; all inner braces are indented.
    match = re.search(r'^' + re.escape(name) + r'\([^;]*?\)\s*\{|^int ' +
                      re.escape(name) + r'\([^;]*?\)\s*\{', web, re.M)
    assert match, name
    end = web.index('\n}', match.end()) + 2
    return web[match.start():end]


for name, mutation in [
    ('do_set_usb_bk_cgi', 'nvram_set('),
    ('do_create_sdn_profile_cgi', 'ret = create_sdn_profile('),
    ('do_create_sdn_legacy_cgi', 'ret = create_sdn_guest_profile('),
    ('do_create_sdn_mwl_cgi', 'ret = create_sdn_mwl_profile('),
    ('do_reset_sdn_rule_cgi', 'nvram_set('),
    ('check_alexa_ifttt_guestnetwork', 'ret = update_alexa_ifttt_guestnetwork('),
]:
    body = function_body(name)
    assert body.index('if (gtbe98_http_locked()') < body.index(mutation), name
print('GT-BE98 dedicated API guard-before-mutation checks passed')

# Execute the real allocation/guard/cleanup portions of every SDN creator.
# Stop at the allocator boundary; no SDN mutation or firmware service is run.
# This catches misplaced guards, undefined exit labels and cleanup leaks.
creators = ['create_alexa_ifttt_sdn_profile', 'create_sdn_profile',
            'create_sdn_mwl_profile', 'create_sdn_guest_profile']
guard_harness = r'''
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#define GTBE98
#define GTBE98_HTTP_SDN_CONFLICT "locked"
typedef struct json_object { int unused; } json_object;
static int locked, mutations, allocations;
static int gtbe98_http_locked(void) { return locked; }
static void logmessage(const char *tag, const char *fmt, ...) { (void)tag; (void)fmt; }
static json_object *json_object_new_object(void) {
    allocations++; return calloc(1, sizeof(json_object));
}
static void json_object_put(json_object *obj) { allocations--; free(obj); }
'''
for name in creators:
    body = function_body(name)
    assert body.count('if (gtbe98_http_locked()') == 1, name
    boundary = body.index('\tif (!web_get_availabel_sdn_profile(')
    labels = re.findall(r'^([A-Za-z_][A-Za-z_0-9]*):', body, re.M)
    assert len(labels) == 1, name
    label = labels[0]
    assert all(target in labels for target in re.findall(r'\bgoto (\w+);', body)), name
    prefix = body[:boundary]
    if not prefix.startswith('int '):
        prefix = 'int ' + prefix
    guard_harness += prefix + '\n mutations++;\n goto ' + label + ';\n' + body[body.index('\n' + label + ':'):]
guard_harness += r'''
int main(void) {
    int i;
    for (i = 0; i < 2; ++i) {
        locked = i; mutations = 0;
        assert(!create_alexa_ifttt_sdn_profile(0, NULL, NULL, 0, 0, NULL, 0));
        assert(!create_sdn_profile(NULL));
        assert(!create_sdn_mwl_profile(NULL));
        assert(!create_sdn_guest_profile(NULL));
        assert(mutations == (locked ? 0 : 4));
        assert(!allocations);
    }
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='gtbe98-sdn-guards-') as work:
    src = Path(work) / 'test.c'
    exe = Path(work) / 'test'
    src.write_text(guard_harness)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu99', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-variable', '-Wno-unused-parameter', str(src), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('GT-BE98 actual SDN creator guards and cleanup passed')
