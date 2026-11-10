#!/usr/bin/env python3
"""Compile actual shared lifecycle hooks with mocked NVRAM/PHY/commands.

This verifies source control flow only; it does not exercise hardware.
Run from any directory: python3 tests/gtbe98_wan_vlan_lifecycle_test.py
"""
from pathlib import Path
import subprocess
import os
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def function(path, signature):
    s=(ROOT / path).read_text(); begin=s.index(signature); start=s.index('{',begin); depth=0
    for end in range(start,len(s)):
        depth += (s[end]=='{')-(s[end]=='}')
        if depth==0:
            return s[begin:end+1]
    raise AssertionError('Unclosed function')

code=r'''
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#define GTBE98 1
#define WAN_UNIT_FIRST 0
static const char *dot1q="", *owned="0", *ready="0", *extwan="0", *active_extwan="0";
static int calls, selected[8], board=1, legacy_status_calls, rtk_status=1, hnd_status=1;
const char *nvram_safe_get(const char *key) {
 if(!strcmp(key,"wan0_dot1q")) return dot1q;
 if(!strcmp(key,"gtbe98_wan_vlan_owned")) return owned;
 if(!strcmp(key,"gtbe98_wan_vlan_ready")) return ready;
 if(!strcmp(key,"wans_extwan")) return extwan;
 if(!strcmp(key,"gtbe98_wan_vlan_extwan")) return active_extwan;
 assert(0); return "";
}
int nvram_match(const char *key,const char *value) {return !strcmp(nvram_safe_get(key),value);}
int is_rtl8372_boardid(void) {return board;}
int rtk_get_phy_status(int port) {assert(port==1);return rtk_status;}
int hnd_get_phy_status(const char *port) {assert(!strcmp(port,"eth0"));return hnd_status;}
int wanport_status_brcm(int unit) {++legacy_status_calls;return 10+unit;}
int mock_eval(const char *cmd,const char *op,...) {
 va_list ap; const char *arg; assert(!strcmp(cmd,"rtkswitch"));
 va_start(ap,op);arg=va_arg(ap,const char *);va_end(ap);
 assert(calls<8);
 if(!strcmp(op,"12")){assert(arg);selected[calls++]=arg[0]-'0';}
 else {assert(!strcmp(op,"5"));selected[calls++]=5;}
 return 0;
}
#define eval(...) mock_eval(__VA_ARGS__,NULL)
'''
code+=function('release/src/router/shared/sysdeps/broadcom/rtkswitch.c','int rtkswitch_LanPort_linkUp(void)')+'\n'
code+=function('release/src/router/shared/boardapi.c','int wanport_status(int wan_unit)')+'\n'
code+=r'''
#include <limits.h>
#define ERP_DBG(...) ((void)0)
#define ERP_WAKEUP 0
#define ERP_STANDBY 1
#define ERP_DEFAULT_COUNT 60
#define ERP_ZERO_STA_COUNT 6
#define MODEL_DSLAC68U 1
static int erp_status, erp_count=1, erp_standbys, erp_checks;
static unsigned int erp_zero_sta_cnt;
static int get_model(void) { return 0; }
static int gtbe98_vlan_requested(void) { return (*dot1q && strcmp(dot1q,"0")) || !strcmp(owned,"1"); }
static int ERP_CHECK_MODEL_LIST(void) { ++erp_checks; return 1; }
static int ERP_CHECK_TCODE_LIST(void) { return 1; }
static int nvram_get_int(const char *key) { return strcmp(key,"Ate_power_on_off_enable") != 0; }
static int erp_check_usb_stat(void) { return 0; }
static int erp_check_wl_stat(int model) { (void)model; return 0; }
static int erp_check_arp_stat(int model) { (void)model; return 0; }
static int erp_check_gphy_stat(int model) { (void)model; return 0; }
static int erp_check_dsl_stat(int model) { (void)model; return 0; }
static int erp_check_wl_auth_stat(void) { return 0; }
static void erp_standby_mode(int model) { (void)model; ++erp_standbys; }
static void erp_wakeup_mode(int model) { (void)model; }
static void doSystem(const char *cmd) { (void)cmd; }
'''
code+=function('release/src/router/rc/erp_monitor.c','static void ERP_CHECK_MODE()')+'\n'
# Compile the real upstream ioctl encoder and dispatcher. Synthetic switch
# tests alone would miss sending raw command numbers to the kernel.
shared = (ROOT / 'release/src/router/shared/shared.h').read_text()
encoder = shared.index('#define RTKSWITCH_IOC_MAGIC')
code += '#include <sys/ioctl.h>\n#include <fcntl.h>\n'
code += shared[encoder:shared.index('\n#endif', encoder)] + '\n'
code += r'''
static unsigned long sent_ioctl;
static unsigned int sent_argument;
static int switch_open(const char *path, int flags) { assert(!strcmp(path,"/dev/rtkswitch") && flags==O_RDONLY); return 42; }
static int switch_close(int fd) { assert(fd==42); return 0; }
static int switch_ioctl(int fd, unsigned long cmd, int *arg) { assert(fd==42); sent_ioctl=cmd; sent_argument=arg?*arg:0; return 0; }
#define RTKSWITCH_DEV "/dev/rtkswitch"
#define open switch_open
#define close switch_close
#define ioctl switch_ioctl
#define dbg(...) ((void)0)
#define usleep(n) ((void)(n))
#define f_write_string(a,b,c,d) ((void)(a),(void)(b),(void)(c),(void)(d))
#define rtkswitch_port_stat(a) ((void)(a),0)
#define rtkswitch_port_mactable(a) ((void)(a),0)
#define rtkswitch_testmode(a,b,c) ((void)(a),(void)(b),(void)(c),0)
#define rtkswitch_regop(a,b,c,d,e) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),0)
'''
code+=function('release/src/router/shared/sysdeps/broadcom/rtkswitch.c','int rtkswitch_ioctl(int val, int val2, int val3, int val4, int val5)')+'\n'
code+='\n#undef open\n#undef close\n#undef ioctl\n'
code+=r'''
int main(void) {
 unsigned int i, commands[]={10,11,12,36,37,38,39,380,3901,3951,3952,3953};
 for(i=0;i<sizeof(commands)/sizeof(commands[0]);++i) {
  assert(!rtkswitch_ioctl(commands[i],0x28,0,0,0));
  assert(_IOC_TYPE(sent_ioctl)=='R' && _IOC_DIR(sent_ioctl)==_IOC_NONE);
  assert((_IOC_NR(sent_ioctl)|(_IOC_SIZE(sent_ioctl)<<8))==commands[i]);
  assert(sent_argument==0x28);
 }
 assert(rtkswitch_LanPort_linkUp()==0 && calls==1 && selected[0]==5);
 dot1q="1"; extwan="1"; calls=0;
 assert(rtkswitch_LanPort_linkUp()==0 && calls==3);
 assert(selected[0]==1 && selected[1]==2 && selected[2]==3);
 dot1q="invalid"; calls=0; assert(rtkswitch_LanPort_linkUp()==0 && calls==3);
 dot1q="0";owned="1";extwan="0";active_extwan="1";calls=0;
 assert(rtkswitch_LanPort_linkUp()==0 && calls==3);
 assert(wanport_status(0)==0 && legacy_status_calls==0);
 ready="1";assert(wanport_status(0)==1);rtk_status=-1;assert(wanport_status(0)==0);
 active_extwan="0";assert(wanport_status(0)==1);hnd_status=-1;assert(wanport_status(0)==0);
 assert(wanport_status(1)==11 && legacy_status_calls==1);
 owned="0";assert(wanport_status(0)==10 && legacy_status_calls==2);
 dot1q="1";ERP_CHECK_MODE();assert(!erp_checks && !erp_standbys && erp_count==1);
 dot1q="invalid";ERP_CHECK_MODE();assert(!erp_checks && !erp_standbys);
 dot1q="0";owned="1";ERP_CHECK_MODE();assert(!erp_checks && !erp_standbys);
 owned="0";ERP_CHECK_MODE();assert(erp_checks==1 && erp_standbys==1);
 puts("GTBE98 lifecycle hook tests passed");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="gtbe98-lifecycle-") as work:
    source = Path(work) / "test.c"
    binary = Path(work) / "test"
    source.write_text(code)
    subprocess.run([os.environ.get("CC", "cc"), "-std=gnu99", "-Wall", "-Wextra", "-Werror",
                    str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
