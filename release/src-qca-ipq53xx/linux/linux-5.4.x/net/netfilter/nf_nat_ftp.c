// SPDX-License-Identifier: GPL-2.0-only
/* FTP extension for TCP NAT alteration. */

/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/inet.h>
#include <linux/tcp.h>
#include <linux/netfilter_ipv4.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_helper.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <linux/netfilter/nf_conntrack_ftp.h>

#define NAT_HELPER_NAME "ftp"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rusty Russell <rusty@rustcorp.com.au>");
MODULE_DESCRIPTION("ftp NAT helper");
MODULE_ALIAS_NF_NAT_HELPER(NAT_HELPER_NAME);

static ushort psid = 0;
module_param(psid, ushort, 0644);
MODULE_PARM_DESC(psid, "MAP_E devices's psid");

static uint psid_len = 0;
module_param(psid_len, uint, 0644);
MODULE_PARM_DESC(psid_len, "MAP_E devices's psid length");

static uint offset = 0;
module_param(offset, uint, 0644);
MODULE_PARM_DESC(offset, "MAP_E devices's psid offset");

/* FIXME: Time out? --RR */

static struct nf_conntrack_nat_helper nat_helper_ftp =
	NF_CT_NAT_HELPER_INIT(NAT_HELPER_NAME);

/**
 * nf_nat_port_valid_check - check the port is in the range of psid
 *   @skb the packets to be translated
 *   @port the port to be checked.
 **/
static int nf_nat_port_valid_check(struct sk_buff *skb, u16 port)
{
	if (psid == 0 || psid_len == 0 || offset == 0)
		return 1;

	if ((psid_len + offset) > 16)
		return 1;

	if ((((port >> (16 - psid_len - offset)) & ((1 << psid_len) - 1))) == psid)
		return 1;

	return 0;
}

static int nf_nat_ftp_fmt_cmd(struct nf_conn *ct, enum nf_ct_ftp_type type,
			      char *buffer, size_t buflen,
			      union nf_inet_addr *addr, u16 port)
{
	switch (type) {
	case NF_CT_FTP_PORT:
	case NF_CT_FTP_PASV:
		return snprintf(buffer, buflen, "%u,%u,%u,%u,%u,%u",
				((unsigned char *)&addr->ip)[0],
				((unsigned char *)&addr->ip)[1],
				((unsigned char *)&addr->ip)[2],
				((unsigned char *)&addr->ip)[3],
				port >> 8,
				port & 0xFF);
	case NF_CT_FTP_EPRT:
		if (nf_ct_l3num(ct) == NFPROTO_IPV4)
			return snprintf(buffer, buflen, "|1|%pI4|%u|",
					&addr->ip, port);
		else
			return snprintf(buffer, buflen, "|2|%pI6|%u|",
					&addr->ip6, port);
	case NF_CT_FTP_EPSV:
		return snprintf(buffer, buflen, "|||%u|", port);
	}

	return 0;
}

/* So, this packet has hit the connection tracking matching code.
   Mangle it, and change the expectation to match the new version. */
static unsigned int nf_nat_ftp(struct sk_buff *skb,
			       enum ip_conntrack_info ctinfo,
			       enum nf_ct_ftp_type type,
			       unsigned int protoff,
			       unsigned int matchoff,
			       unsigned int matchlen,
			       struct nf_conntrack_expect *exp)
{
	union nf_inet_addr newaddr;
	u16 port;
	int dir = CTINFO2DIR(ctinfo);
	struct nf_conn *ct = exp->master;
	char buffer[sizeof("|1||65535|") + INET6_ADDRSTRLEN];
	unsigned int buflen;

	pr_debug("type %i, off %u len %u\n", type, matchoff, matchlen);

	/* Connection will come from wherever this packet goes, hence !dir */
	newaddr = ct->tuplehash[!dir].tuple.dst.u3;
	exp->saved_proto.tcp.port = exp->tuple.dst.u.tcp.port;
	exp->dir = !dir;

	/* When you see the packet, we need to NAT it the same as the
	 * this one. */
	exp->expectfn = nf_nat_follow_master;

	/* In the case of MAP-E, the FTP ALG source port number must use its own
	 * PSID. Otherwise the returned packets from ftp server will use other
	 * than its own IPv6 address.
	 * so let the check hook to validate the port*/
	for (port = ntohs(exp->saved_proto.tcp.port); port != 0; port++) {
		int ret;

		if (!nf_nat_port_valid_check(skb, port))
			continue;

		exp->tuple.dst.u.tcp.port = htons(port);
		ret = nf_ct_expect_related(exp, 0);
		if (ret == 0)
			break;
		else if (ret != -EBUSY) {
			port = 0;
			break;
		}
	}

	if (port == 0) {
		nf_ct_helper_log(skb, ct, "all ports in use");
		return NF_DROP;
	}

	buflen = nf_nat_ftp_fmt_cmd(ct, type, buffer, sizeof(buffer),
				    &newaddr, port);
	if (!buflen)
		goto out;

	pr_debug("calling nf_nat_mangle_tcp_packet\n");

	if (!nf_nat_mangle_tcp_packet(skb, ct, ctinfo, protoff, matchoff,
				      matchlen, buffer, buflen))
		goto out;

	return NF_ACCEPT;

out:
	nf_ct_helper_log(skb, ct, "cannot mangle packet");
	nf_ct_unexpect_related(exp);
	return NF_DROP;
}

static void __exit nf_nat_ftp_fini(void)
{
	nf_nat_helper_unregister(&nat_helper_ftp);
	RCU_INIT_POINTER(nf_nat_ftp_hook, NULL);
	synchronize_rcu();
}

static int __init nf_nat_ftp_init(void)
{
	BUG_ON(nf_nat_ftp_hook != NULL);
	nf_nat_helper_register(&nat_helper_ftp);
	RCU_INIT_POINTER(nf_nat_ftp_hook, nf_nat_ftp);
	return 0;
}

/* Prior to 2.6.11, we had a ports param.  No longer, but don't break users. */
static int warn_set(const char *val, const struct kernel_param *kp)
{
	pr_info("kernel >= 2.6.10 only uses 'ports' for conntrack modules\n");
	return 0;
}
module_param_call(ports, warn_set, NULL, NULL, 0);

module_init(nf_nat_ftp_init);
module_exit(nf_nat_ftp_fini);
