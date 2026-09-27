/*
 * m_police.c		Parse/print policing module options.
 *
 *		This program is free software; you can u32istribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 * FIXES:       19990619 - J Hadi Salim (hadi@cyberus.ca)
 *		simple addattr packaging fix.
 *		2002: J Hadi Salim - Add tc action extensions syntax
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "utils.h"
#include "tc_util.h"

static int act_parse_police(struct action_util *a, int *argc_p,
			    char ***argv_p, int tca_id, struct nlmsghdr *n);
static int print_police(struct action_util *a, FILE *f, struct rtattr *tb);

struct action_util police_action_util = {
	.id = "police",
	.parse_aopt = act_parse_police,
	.print_aopt = print_police,
};

static void usage(void)
{
	fprintf(stderr,
		"Usage: ... police [ rate BPS burst BYTES[/BYTES] ] \n"
		"		[ pkts_rate RATE pkts_burst PACKETS ] [ mtu BYTES[/BYTES] ]\n"
		"		[ peakrate BPS ] [ avrate BPS ] [ overhead BYTES ]\n"
		"		[ linklayer TYPE ] [ CONTROL ]\n"
		"Where: CONTROL := conform-exceed <EXCEEDACT>[/NOTEXCEEDACT]\n"
		"		  Define how to handle packets which exceed (<EXCEEDACT>)\n"
		"		  or conform (<NOTEXCEEDACT>) the configured bandwidth limit.\n"
		"       EXCEEDACT/NOTEXCEEDACT := { pipe | ok | reclassify | drop | continue |\n"
		"				   goto chain <CHAIN_INDEX> }\n");
	exit(-1);
}

static int act_parse_police(struct action_util *a, int *argc_p, char ***argv_p,
			    int tca_id, struct nlmsghdr *n)
{
	int argc = *argc_p;
	char **argv = *argv_p;
	int res = -1;
	int ok = 0;
	struct tc_police p = { .action = TC_POLICE_RECLASSIFY };
	__u32 rtab[256];
	__u32 ptab[256];
	__u32 avrate = 0;
	int presult = 0;
	unsigned buffer = 0, mtu = 0, mpu = 0;
	unsigned short overhead = 0;
	unsigned int linklayer = LINKLAYER_ETHERNET; /* Assume ethernet */
	int Rcell_log =  -1, Pcell_log = -1;
	struct rtattr *tail;
	__u64 rate64 = 0, prate64 = 0;
	__u64 pps64 = 0, ppsburst64 = 0;

	if (a) /* new way of doing things */
		NEXT_ARG();

	if (argc <= 0)
		return -1;

	while (argc > 0) {

		if (matches(*argv, "index") == 0) {
			NEXT_ARG();
			if (get_u32(&p.index, *argv, 10))
				invarg("index", *argv);
		} else if (matches(*argv, "burst") == 0 ||
			strcmp(*argv, "buffer") == 0 ||
			strcmp(*argv, "maxburst") == 0) {
			NEXT_ARG();
			if (buffer)
				duparg("buffer/burst", *argv);
			if (get_size_and_cell(&buffer, &Rcell_log, *argv) < 0)
				invarg("buffer", *argv);
		} else if (strcmp(*argv, "mtu") == 0 ||
			   strcmp(*argv, "minburst") == 0) {
			NEXT_ARG();
			if (mtu)
				duparg("mtu/minburst", *argv);
			if (get_size_and_cell(&mtu, &Pcell_log, *argv) < 0)
				invarg("mtu", *argv);
		} else if (strcmp(*argv, "mpu") == 0) {
			NEXT_ARG();
			if (mpu)
				duparg("mpu", *argv);
			if (get_size(&mpu, *argv))
				invarg("mpu", *argv);
		} else if (strcmp(*argv, "rate") == 0) {
			NEXT_ARG();
			if (rate64)
				duparg("rate", *argv);
			if (get_rate64(&rate64, *argv))
				invarg("rate", *argv);
		} else if (strcmp(*argv, "avrate") == 0) {
			NEXT_ARG();
			if (avrate)
				duparg("avrate", *argv);
			if (get_rate(&avrate, *argv))
				invarg("avrate", *argv);
		} else if (matches(*argv, "peakrate") == 0) {
			NEXT_ARG();
			if (prate64)
				duparg("peakrate", *argv);
			if (get_rate64(&prate64, *argv))
				invarg("peakrate", *argv);
		} else if (matches(*argv, "reclassify") == 0 ||
			   matches(*argv, "drop") == 0 ||
			   matches(*argv, "shot") == 0 ||
			   matches(*argv, "continue") == 0 ||
			   matches(*argv, "pass") == 0 ||
			   matches(*argv, "ok") == 0 ||
			   matches(*argv, "pipe") == 0 ||
			   matches(*argv, "goto") == 0) {
			if (!parse_action_control(&argc, &argv, &p.action, false))
				goto action_ctrl_ok;
			return -1;
		} else if (strcmp(*argv, "conform-exceed") == 0) {
			NEXT_ARG();
			if (!parse_action_control_slash(&argc, &argv, &p.action,
							&presult, true))
				goto action_ctrl_ok;
			return -1;
		} else if (matches(*argv, "overhead") == 0) {
			NEXT_ARG();
			if (get_u16(&overhead, *argv, 10))
				invarg("overhead", *argv);
		} else if (matches(*argv, "linklayer") == 0) {
			NEXT_ARG();
			if (get_linklayer(&linklayer, *argv))
				invarg("linklayer", *argv);
		} else if (matches(*argv, "pkts_rate") == 0) {
			NEXT_ARG();
			if (pps64)
				duparg("pkts_rate", *argv);
			if (get_u64(&pps64, *argv, 10))
				invarg("pkts_rate", *argv);
		} else if (matches(*argv, "pkts_burst") == 0) {
			NEXT_ARG();
			if (ppsburst64)
				duparg("pkts_burst", *argv);
			if (get_u64(&ppsburst64, *argv, 10))
				invarg("pkts_burst", *argv);
		} else if (strcmp(*argv, "help") == 0) {
			usage();
		} else {
			break;
		}
		NEXT_ARG_FWD();
action_ctrl_ok:
		ok++;
	}

	if (!ok)
		return -1;

	if (rate64 && avrate)
		return -1;

	/* Must at least do late binding, use TB or ewma policing */
	if (!rate64 && !avrate && !p.index && !mtu && !pps64) {
		fprintf(stderr, "'rate' or 'avrate' or 'mtu' or 'pkts_rate' MUST be specified.\n");
		return -1;
	}

	/* When the TB policer is used, burst is required */
	if (rate64 && !buffer && !avrate) {
		fprintf(stderr, "'burst' requires 'rate'.\n");
		return -1;
	}

	/* When the packets TB policer is used, pkts_burst is required */
	if (pps64 && !ppsburst64) {
		fprintf(stderr, "'pkts_burst' requires 'pkts_rate'.\n");
		return -1;
	}

	/* forbid rate and pkts_rate in same action */
	if (pps64 && rate64) {
		fprintf(stderr, "'rate' and 'pkts_rate' are not allowed in same action.\n");
		return -1;
	}

	if (prate64) {
		if (!rate64) {
			fprintf(stderr, "'peakrate' requires 'rate'.\n");
			return -1;
		}
		if (!mtu) {
			fprintf(stderr, "'mtu' is required, if 'peakrate' is requested.\n");
			return -1;
		}
	}

	if (rate64) {
		p.rate.rate = (rate64 >= (1ULL << 32)) ? ~0U : rate64;
		p.rate.mpu = mpu;
		p.rate.overhead = overhead;
		if (tc_calc_rtable_64(&p.rate, rtab, Rcell_log, mtu,
				   linklayer, rate64) < 0) {
			fprintf(stderr, "POLICE: failed to calculate rate table.\n");
			return -1;
		}
		p.burst = tc_calc_xmittime(rate64, buffer);
	}
	p.mtu = mtu;
	if (prate64) {
		p.peakrate.rate = (prate64 >= (1ULL << 32)) ? ~0U : prate64;
		p.peakrate.mpu = mpu;
		p.peakrate.overhead = overhead;
		if (tc_calc_rtable_64(&p.peakrate, ptab, Pcell_log, mtu,
				   linklayer, prate64) < 0) {
			fprintf(stderr, "POLICE: failed to calculate peak rate table.\n");
			return -1;
		}
	}

	tail = addattr_nest(n, MAX_MSG, tca_id);
	addattr_l(n, MAX_MSG, TCA_POLICE_TBF, &p, sizeof(p));
	if (rate64) {
		addattr_l(n, MAX_MSG, TCA_POLICE_RATE, rtab, 1024);
		if (rate64 >= (1ULL << 32))
			addattr64(n, MAX_MSG, TCA_POLICE_RATE64, rate64);
	}
	if (prate64) {
		addattr_l(n, MAX_MSG, TCA_POLICE_PEAKRATE, ptab, 1024);
		if (prate64 >= (1ULL << 32))
			addattr64(n, MAX_MSG, TCA_POLICE_PEAKRATE64, prate64);
	}
	if (avrate)
		addattr32(n, MAX_MSG, TCA_POLICE_AVRATE, avrate);
	if (presult)
		addattr32(n, MAX_MSG, TCA_POLICE_RESULT, presult);

	if (pps64) {
		addattr64(n, MAX_MSG, TCA_POLICE_PKTRATE64, pps64);
		ppsburst64 = tc_calc_xmittime(pps64, ppsburst64);
		addattr64(n, MAX_MSG, TCA_POLICE_PKTBURST64, ppsburst64);
	}

	addattr_nest_end(n, tail);
	res = 0;

	*argc_p = argc;
	*argv_p = argv;
	return res;
}

int parse_police(int *argc_p, char ***argv_p, int tca_id, struct nlmsghdr *n)
{
	return act_parse_police(NULL, argc_p, argv_p, tca_id, n);
}

static int print_police(struct action_util *a, FILE *f, struct rtattr *arg)
{
	SPRINT_BUF(b2);
	struct tc_police *p;
	struct rtattr *tb[TCA_POLICE_MAX+1];
	unsigned int buffer;
	unsigned int linklayer;
	__u64 rate64, prate64;
	__u64 pps64, ppsburst64;

	print_string(PRINT_JSON, "kind", "%s", "police");
	if (arg == NULL)
		return 0;

	parse_rtattr_nested(tb, TCA_POLICE_MAX, arg);

	if (tb[TCA_POLICE_TBF] == NULL) {
		fprintf(stderr, "[NULL police tbf]");
		return -1;
	}
#ifndef STOOPID_8BYTE
	if (RTA_PAYLOAD(tb[TCA_POLICE_TBF])  < sizeof(*p)) {
		fprintf(stderr, "[truncated police tbf]");
		return -1;
	}
#endif
	p = RTA_DATA(tb[TCA_POLICE_TBF]);

	rate64 = p->rate.rate;
	if (tb[TCA_POLICE_RATE64] &&
	    RTA_PAYLOAD(tb[TCA_POLICE_RATE64]) >= sizeof(rate64))
		rate64 = rta_getattr_u64(tb[TCA_POLICE_RATE64]);

	print_hex(PRINT_FP, NULL, " police 0x%x ", p->index);
	print_uint(PRINT_JSON, "index", NULL, p->index);
	tc_print_rate(PRINT_FP, NULL, "rate %s ", rate64);
	buffer = tc_calc_xmitsize(rate64, p->burst);
	print_size(PRINT_FP, NULL, "burst %s ", buffer);
	print_size(PRINT_FP, NULL, "mtu %s ", p->mtu);
	if (show_raw)
		print_hex(PRINT_FP, NULL, "[%08x] ", p->burst);

	prate64 = p->peakrate.rate;
	if (tb[TCA_POLICE_PEAKRATE64] &&
	    RTA_PAYLOAD(tb[TCA_POLICE_PEAKRATE64]) >= sizeof(prate64))
		prate64 = rta_getattr_u64(tb[TCA_POLICE_PEAKRATE64]);

	if (prate64)
		tc_print_rate(PRINT_FP, NULL, "peakrate %s ", prate64);

	if (tb[TCA_POLICE_AVRATE])
		tc_print_rate(PRINT_FP, NULL, "avrate %s ",
			      rta_getattr_u32(tb[TCA_POLICE_AVRATE]));

	if ((tb[TCA_POLICE_PKTRATE64] &&
	     RTA_PAYLOAD(tb[TCA_POLICE_PKTRATE64]) >= sizeof(pps64)) &&
	     (tb[TCA_POLICE_PKTBURST64] &&
	      RTA_PAYLOAD(tb[TCA_POLICE_PKTBURST64]) >= sizeof(ppsburst64))) {
		pps64 = rta_getattr_u64(tb[TCA_POLICE_PKTRATE64]);
		ppsburst64 = rta_getattr_u64(tb[TCA_POLICE_PKTBURST64]);
		ppsburst64 = tc_calc_xmitsize(pps64, ppsburst64);
		print_u64(PRINT_ANY, "pkts_rate", "pkts_rate %llu ", pps64);
		print_u64(PRINT_ANY, "pkts_burst", "pkts_burst %llu ", ppsburst64);
	}

	print_action_control(f, "action ", p->action, "");

	if (tb[TCA_POLICE_RESULT]) {
		__u32 action = rta_getattr_u32(tb[TCA_POLICE_RESULT]);

		print_action_control(f, "/", action, " ");
	} else {
		print_string(PRINT_FP, NULL, " ", NULL);
	}

	print_size(PRINT_ANY, "overhead", "overhead %s ", p->rate.overhead);
	linklayer = (p->rate.linklayer & TC_LINKLAYER_MASK);
	if (linklayer > TC_LINKLAYER_ETHERNET || show_details)
		print_string(PRINT_ANY, "linklayer", "linklayer %s ",
			     sprint_linklayer(linklayer, b2));
	print_nl();
	print_int(PRINT_ANY, "ref", "\tref %d ", p->refcnt);
	print_int(PRINT_ANY, "bind", "bind %d ", p->bindcnt);
	if (show_stats) {
		if (tb[TCA_POLICE_TM]) {
			struct tcf_t *tm = RTA_DATA(tb[TCA_POLICE_TM]);

			print_tm(f, tm);
		}
	}
	print_nl();


	return 0;
}

int tc_print_police(FILE *f, struct rtattr *arg)
{
	return print_police(&police_action_util, f, arg);
}
