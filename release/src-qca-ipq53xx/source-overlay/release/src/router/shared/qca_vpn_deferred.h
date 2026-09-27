#ifndef _QCA_VPN_DEFERRED_H_
#define _QCA_VPN_DEFERRED_H_

#include <net/if.h>
#include <netinet/in.h>

/* Raw Fusion/SDN indices are not explicit Merlin Director targets. */
int qca_vpn_binding_deferred(int index);
/* Validate original SDN/default fields before atoi can hide invalid values.
 * Returns 0 for independent/disabled/WAN, 1 for a retained positive target,
 * or 2 for malformed active/identity/target fields. All nonzero values defer. */
int qca_vpn_sdn_deferred(int sdn_idx, int parsed_target);

struct qca_vpn_deferred_policy {
	int family, index;
	unsigned int flags;
	char src[INET6_ADDRSTRLEN + 4], dst[INET6_ADDRSTRLEN + 4];
	char iif[IFNAMSIZ];
};

#define QCA_VPN_DEFERRED_NETWORK 1U
#define QCA_VPN_DEFERRED_DNS_NETWORK 2U

/* Read original NVRAM selectors without legacy VPNC_DEV_POLICY truncation.
 * AF_UNSPEC means both families. Empty selectors mean all LAN ingress.
 * NETWORK blocks whole ingress traffic and DNS; DNS_NETWORK blocks ingress
 * DNS only. These flags ignore src/dst; empty iif means every configured LAN
 * and SDN ingress, never WAN. index is -1 when malformed and must not be used
 * to infer an old routing table. All callbacks form the current conservative
 * plan, including malformed records and IPv4 clients without IPv6 identity.
 * Returns 0 for valid records, 1 for malformed records (their conservative
 * scopes are still delivered), or -1 for a resource/callback failure. */
int qca_vpn_deferred_foreach(int (*callback)(const struct qca_vpn_deferred_policy *, void *), void *arg);

/* Legacy source and interface routes are independent branches. A matching
 * enabled stock WAN branch must survive cleanup of ambiguous main lookups.
 * Returns 1 for a valid match, 0 for none, -1 for a resource failure. */
int qca_vpn_deferred_wan_override(const char *src, const char *iif);

#endif
