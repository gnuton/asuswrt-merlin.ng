/* Unrelated branches in handle_sdn_feature must never run in this fixture. */
#include <stdio.h>
#include <stdlib.h>
#define TRAP(name) void name(void) { fprintf(stderr, "Unexpected deferred dependency: " #name "\n"); abort(); }
TRAP(mkdir_if_none)
TRAP(handle_SDN_internal_access)
TRAP(update_wgc_by_sdn)
TRAP(update_wgs_by_sdn)
TRAP(update_ipsec_server_by_sdn)
TRAP(reset_sdn_firewall)
TRAP(create_iptables_file)
TRAP(get_dns_filter)
TRAP(write_URLFilter_SDN)
TRAP(write_NwServiceFilter_SDN)
TRAP(close_n_restore_iptables_file)
TRAP(handle_URLFilter_jump_rule)
TRAP(handle_NwServiceFilter_jump_rule)
