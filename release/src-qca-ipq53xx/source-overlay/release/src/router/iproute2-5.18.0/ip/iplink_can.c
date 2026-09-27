/*
 * iplink_can.c	CAN device support
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Wolfgang Grandegger <wg@grandegger.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/can/netlink.h>

#include "rt_names.h"
#include "utils.h"
#include "ip_common.h"

static void print_usage(FILE *f)
{
	fprintf(f,
		"Usage: ip link set DEVICE type can\n"
		"\t[ bitrate BITRATE [ sample-point SAMPLE-POINT] ] |\n"
		"\t[ tq TQ prop-seg PROP_SEG phase-seg1 PHASE-SEG1\n \t  phase-seg2 PHASE-SEG2 [ sjw SJW ] ]\n"
		"\n"
		"\t[ dbitrate BITRATE [ dsample-point SAMPLE-POINT] ] |\n"
		"\t[ dtq TQ dprop-seg PROP_SEG dphase-seg1 PHASE-SEG1\n \t  dphase-seg2 PHASE-SEG2 [ dsjw SJW ] ]\n"
		"\t[ tdcv TDCV tdco TDCO tdcf TDCF ]\n"
		"\n"
		"\t[ loopback { on | off } ]\n"
		"\t[ listen-only { on | off } ]\n"
		"\t[ triple-sampling { on | off } ]\n"
		"\t[ one-shot { on | off } ]\n"
		"\t[ berr-reporting { on | off } ]\n"
		"\t[ fd { on | off } ]\n"
		"\t[ fd-non-iso { on | off } ]\n"
		"\t[ presume-ack { on | off } ]\n"
		"\t[ cc-len8-dlc { on | off } ]\n"
		"\t[ tdc-mode { auto | manual | off } ]\n"
		"\n"
		"\t[ restart-ms TIME-MS ]\n"
		"\t[ restart ]\n"
		"\n"
		"\t[ termination { 0..65535 } ]\n"
		"\n"
		"\tWhere: BITRATE	:= { NUMBER in bps }\n"
		"\t	  SAMPLE-POINT	:= { 0.000..0.999 }\n"
		"\t	  TQ		:= { NUMBER in ns }\n"
		"\t	  PROP-SEG	:= { NUMBER in tq }\n"
		"\t	  PHASE-SEG1	:= { NUMBER in tq }\n"
		"\t	  PHASE-SEG2	:= { NUMBER in tq }\n"
		"\t	  SJW		:= { NUMBER in tq }\n"
		"\t	  TDCV		:= { NUMBER in tc }\n"
		"\t	  TDCO		:= { NUMBER in tc }\n"
		"\t	  TDCF		:= { NUMBER in tc }\n"
		"\t	  RESTART-MS	:= { 0 | NUMBER in ms }\n"
		);
}

static void usage(void)
{
	print_usage(stderr);
}

static int get_float(float *val, const char *arg)
{
	float res;
	char *ptr;

	if (!arg || !*arg)
		return -1;
	res = strtof(arg, &ptr);
	if (!ptr || ptr == arg || *ptr)
		return -1;
	*val = res;
	return 0;
}

static void set_ctrlmode(char *name, char *arg,
			 struct can_ctrlmode *cm, __u32 flags)
{
	if (strcmp(arg, "on") == 0) {
		cm->flags |= flags;
	} else if (strcmp(arg, "off") != 0) {
		fprintf(stderr,
			"Error: argument of \"%s\" must be \"on\" or \"off\", not \"%s\"\n",
			name, arg);
		exit(-1);
	}
	cm->mask |= flags;
}

static void print_flag(enum output_type t, __u32 *flags, __u32 flag,
		       const char* name)
{
	if (*flags & flag) {
		*flags &= ~flag;
		print_string(t, NULL, *flags ? "%s," : "%s", name);
	}
}

static void print_ctrlmode(enum output_type t, __u32 flags, const char* key)
{
	if (!flags)
		return;

	open_json_array(t, is_json_context() ? key : "<");

	print_flag(t, &flags, CAN_CTRLMODE_LOOPBACK, "LOOPBACK");
	print_flag(t, &flags, CAN_CTRLMODE_LISTENONLY, "LISTEN-ONLY");
	print_flag(t, &flags, CAN_CTRLMODE_3_SAMPLES, "TRIPLE-SAMPLING");
	print_flag(t, &flags, CAN_CTRLMODE_ONE_SHOT, "ONE-SHOT");
	print_flag(t, &flags, CAN_CTRLMODE_BERR_REPORTING, "BERR-REPORTING");
	print_flag(t, &flags, CAN_CTRLMODE_FD, "FD");
	print_flag(t, &flags, CAN_CTRLMODE_FD_NON_ISO, "FD-NON-ISO");
	print_flag(t, &flags, CAN_CTRLMODE_PRESUME_ACK, "PRESUME-ACK");
	print_flag(t, &flags, CAN_CTRLMODE_CC_LEN8_DLC, "CC-LEN8-DLC");
	print_flag(t, &flags, CAN_CTRLMODE_TDC_AUTO, "TDC-AUTO");
	print_flag(t, &flags, CAN_CTRLMODE_TDC_MANUAL, "TDC-MANUAL");

	if (flags)
		print_hex(t, NULL, "%x", flags);

	close_json_array(t, "> ");
}

static int can_parse_opt(struct link_util *lu, int argc, char **argv,
			 struct nlmsghdr *n)
{
	struct can_bittiming bt = {}, dbt = {};
	struct can_ctrlmode cm = { 0 };
	struct rtattr *tdc;
	__u32 tdcv = -1, tdco = -1, tdcf = -1;

	while (argc > 0) {
		if (matches(*argv, "bitrate") == 0) {
			NEXT_ARG();
			if (get_u32(&bt.bitrate, *argv, 0))
				invarg("invalid \"bitrate\" value\n", *argv);
		} else if (matches(*argv, "sample-point") == 0) {
			float sp;

			NEXT_ARG();
			if (get_float(&sp, *argv))
				invarg("invalid \"sample-point\" value\n",
				       *argv);
			bt.sample_point = (__u32)(sp * 1000);
		} else if (matches(*argv, "tq") == 0) {
			NEXT_ARG();
			if (get_u32(&bt.tq, *argv, 0))
				invarg("invalid \"tq\" value\n", *argv);
		} else if (matches(*argv, "prop-seg") == 0) {
			NEXT_ARG();
			if (get_u32(&bt.prop_seg, *argv, 0))
				invarg("invalid \"prop-seg\" value\n", *argv);
		} else if (matches(*argv, "phase-seg1") == 0) {
			NEXT_ARG();
			if (get_u32(&bt.phase_seg1, *argv, 0))
				invarg("invalid \"phase-seg1\" value\n", *argv);
		} else if (matches(*argv, "phase-seg2") == 0) {
			NEXT_ARG();
			if (get_u32(&bt.phase_seg2, *argv, 0))
				invarg("invalid \"phase-seg2\" value\n", *argv);
		} else if (matches(*argv, "sjw") == 0) {
			NEXT_ARG();
			if (get_u32(&bt.sjw, *argv, 0))
				invarg("invalid \"sjw\" value\n", *argv);
		} else if (matches(*argv, "dbitrate") == 0) {
			NEXT_ARG();
			if (get_u32(&dbt.bitrate, *argv, 0))
				invarg("invalid \"dbitrate\" value\n", *argv);
		} else if (matches(*argv, "dsample-point") == 0) {
			float sp;

			NEXT_ARG();
			if (get_float(&sp, *argv))
				invarg("invalid \"dsample-point\" value\n", *argv);
			dbt.sample_point = (__u32)(sp * 1000);
		} else if (matches(*argv, "dtq") == 0) {
			NEXT_ARG();
			if (get_u32(&dbt.tq, *argv, 0))
				invarg("invalid \"dtq\" value\n", *argv);
		} else if (matches(*argv, "dprop-seg") == 0) {
			NEXT_ARG();
			if (get_u32(&dbt.prop_seg, *argv, 0))
				invarg("invalid \"dprop-seg\" value\n", *argv);
		} else if (matches(*argv, "dphase-seg1") == 0) {
			NEXT_ARG();
			if (get_u32(&dbt.phase_seg1, *argv, 0))
				invarg("invalid \"dphase-seg1\" value\n", *argv);
		} else if (matches(*argv, "dphase-seg2") == 0) {
			NEXT_ARG();
			if (get_u32(&dbt.phase_seg2, *argv, 0))
				invarg("invalid \"dphase-seg2\" value\n", *argv);
		} else if (matches(*argv, "dsjw") == 0) {
			NEXT_ARG();
			if (get_u32(&dbt.sjw, *argv, 0))
				invarg("invalid \"dsjw\" value\n", *argv);
		} else if (matches(*argv, "tdcv") == 0) {
			NEXT_ARG();
			if (get_u32(&tdcv, *argv, 0))
				invarg("invalid \"tdcv\" value\n", *argv);
		} else if (matches(*argv, "tdco") == 0) {
			NEXT_ARG();
			if (get_u32(&tdco, *argv, 0))
				invarg("invalid \"tdco\" value\n", *argv);
		} else if (matches(*argv, "tdcf") == 0) {
			NEXT_ARG();
			if (get_u32(&tdcf, *argv, 0))
				invarg("invalid \"tdcf\" value\n", *argv);
		} else if (matches(*argv, "loopback") == 0) {
			NEXT_ARG();
			set_ctrlmode("loopback", *argv, &cm,
				     CAN_CTRLMODE_LOOPBACK);
		} else if (matches(*argv, "listen-only") == 0) {
			NEXT_ARG();
			set_ctrlmode("listen-only", *argv, &cm,
				     CAN_CTRLMODE_LISTENONLY);
		} else if (matches(*argv, "triple-sampling") == 0) {
			NEXT_ARG();
			set_ctrlmode("triple-sampling", *argv, &cm,
				     CAN_CTRLMODE_3_SAMPLES);
		} else if (matches(*argv, "one-shot") == 0) {
			NEXT_ARG();
			set_ctrlmode("one-shot", *argv, &cm,
				     CAN_CTRLMODE_ONE_SHOT);
		} else if (matches(*argv, "berr-reporting") == 0) {
			NEXT_ARG();
			set_ctrlmode("berr-reporting", *argv, &cm,
				     CAN_CTRLMODE_BERR_REPORTING);
		} else if (matches(*argv, "fd") == 0) {
			NEXT_ARG();
			set_ctrlmode("fd", *argv, &cm,
				     CAN_CTRLMODE_FD);
		} else if (matches(*argv, "fd-non-iso") == 0) {
			NEXT_ARG();
			set_ctrlmode("fd-non-iso", *argv, &cm,
				     CAN_CTRLMODE_FD_NON_ISO);
		} else if (matches(*argv, "presume-ack") == 0) {
			NEXT_ARG();
			set_ctrlmode("presume-ack", *argv, &cm,
				     CAN_CTRLMODE_PRESUME_ACK);
		} else if (matches(*argv, "cc-len8-dlc") == 0) {
			NEXT_ARG();
			set_ctrlmode("cc-len8-dlc", *argv, &cm,
				     CAN_CTRLMODE_CC_LEN8_DLC);
		} else if (matches(*argv, "tdc-mode") == 0) {
			NEXT_ARG();
			if (strcmp(*argv, "auto") == 0) {
				cm.flags |= CAN_CTRLMODE_TDC_AUTO;
				cm.mask |= CAN_CTRLMODE_TDC_AUTO;
			} else if (strcmp(*argv, "manual") == 0) {
				cm.flags |= CAN_CTRLMODE_TDC_MANUAL;
				cm.mask |= CAN_CTRLMODE_TDC_MANUAL;
			} else if (strcmp(*argv, "off") == 0) {
				cm.mask |= CAN_CTRLMODE_TDC_AUTO |
					   CAN_CTRLMODE_TDC_MANUAL;
			} else {
				fprintf(stderr,
					"Error: argument of \"tdc-mode\" must be \"auto\", \"manual\" or \"off\", not \"%s\"\n",
					*argv);
				exit (-1);
			}
		} else if (matches(*argv, "restart") == 0) {
			__u32 val = 1;

			addattr32(n, 1024, IFLA_CAN_RESTART, val);
		} else if (matches(*argv, "restart-ms") == 0) {
			__u32 val;

			NEXT_ARG();
			if (get_u32(&val, *argv, 0))
				invarg("invalid \"restart-ms\" value\n", *argv);
			addattr32(n, 1024, IFLA_CAN_RESTART_MS, val);
		} else if (matches(*argv, "termination") == 0) {
			__u16 val;

			NEXT_ARG();
			if (get_u16(&val, *argv, 0))
				invarg("invalid \"termination\" value\n",
				       *argv);
			addattr16(n, 1024, IFLA_CAN_TERMINATION, val);
		} else if (matches(*argv, "help") == 0) {
			usage();
			return -1;
		} else {
			fprintf(stderr, "can: unknown option \"%s\"\n", *argv);
			usage();
			return -1;
		}
		argc--, argv++;
	}

	if (bt.bitrate || bt.tq)
		addattr_l(n, 1024, IFLA_CAN_BITTIMING, &bt, sizeof(bt));
	if (dbt.bitrate || dbt.tq)
		addattr_l(n, 1024, IFLA_CAN_DATA_BITTIMING, &dbt, sizeof(dbt));
	if (cm.mask)
		addattr_l(n, 1024, IFLA_CAN_CTRLMODE, &cm, sizeof(cm));

	if (tdcv != -1 || tdco != -1 || tdcf != -1) {
		tdc = addattr_nest(n, 1024, IFLA_CAN_TDC | NLA_F_NESTED);
		if (tdcv != -1)
			addattr32(n, 1024, IFLA_CAN_TDC_TDCV, tdcv);
		if (tdco != -1)
			addattr32(n, 1024, IFLA_CAN_TDC_TDCO, tdco);
		if (tdcf != -1)
			addattr32(n, 1024, IFLA_CAN_TDC_TDCF, tdcf);
		addattr_nest_end(n, tdc);
	}

	return 0;
}

static const char *can_state_names[CAN_STATE_MAX] = {
	[CAN_STATE_ERROR_ACTIVE] = "ERROR-ACTIVE",
	[CAN_STATE_ERROR_WARNING] = "ERROR-WARNING",
	[CAN_STATE_ERROR_PASSIVE] = "ERROR-PASSIVE",
	[CAN_STATE_BUS_OFF] = "BUS-OFF",
	[CAN_STATE_STOPPED] = "STOPPED",
	[CAN_STATE_SLEEPING] = "SLEEPING"
};

static void can_print_nl_indent(void)
{
	print_nl();
	print_string(PRINT_FP, NULL, "%s", "\t ");
}

static void __attribute__((format(printf, 2, 0)))
can_print_timing_min_max(const char *json_attr, const char *fp_attr,
			 int min, int max)
{
	print_null(PRINT_FP, NULL, fp_attr, NULL);
	open_json_object(json_attr);
	print_uint(PRINT_ANY, "min", " %d", min);
	print_uint(PRINT_ANY, "max", "..%d", max);
	close_json_object();
}

static void can_print_tdc_opt(FILE *f, struct rtattr *tdc_attr)
{
	struct rtattr *tb[IFLA_CAN_TDC_MAX + 1];

	parse_rtattr_nested(tb, IFLA_CAN_TDC_MAX, tdc_attr);
	if (tb[IFLA_CAN_TDC_TDCV] || tb[IFLA_CAN_TDC_TDCO] ||
	    tb[IFLA_CAN_TDC_TDCF]) {
		open_json_object("tdc");
		can_print_nl_indent();
		if (tb[IFLA_CAN_TDC_TDCV]) {
			__u32 *tdcv = RTA_DATA(tb[IFLA_CAN_TDC_TDCV]);

			print_uint(PRINT_ANY, "tdcv", " tdcv %u", *tdcv);
		}
		if (tb[IFLA_CAN_TDC_TDCO]) {
			__u32 *tdco = RTA_DATA(tb[IFLA_CAN_TDC_TDCO]);

			print_uint(PRINT_ANY, "tdco", " tdco %u", *tdco);
		}
		if (tb[IFLA_CAN_TDC_TDCF]) {
			__u32 *tdcf = RTA_DATA(tb[IFLA_CAN_TDC_TDCF]);

			print_uint(PRINT_ANY, "tdcf", " tdcf %u", *tdcf);
		}
		close_json_object();
	}
}

static void can_print_tdc_const_opt(FILE *f, struct rtattr *tdc_attr)
{
	struct rtattr *tb[IFLA_CAN_TDC_MAX + 1];

	parse_rtattr_nested(tb, IFLA_CAN_TDC_MAX, tdc_attr);
	open_json_object("tdc");
	can_print_nl_indent();
	if (tb[IFLA_CAN_TDC_TDCV_MIN] && tb[IFLA_CAN_TDC_TDCV_MAX]) {
		__u32 *tdcv_min = RTA_DATA(tb[IFLA_CAN_TDC_TDCV_MIN]);
		__u32 *tdcv_max = RTA_DATA(tb[IFLA_CAN_TDC_TDCV_MAX]);

		can_print_timing_min_max("tdcv", " tdcv", *tdcv_min, *tdcv_max);
	}
	if (tb[IFLA_CAN_TDC_TDCO_MIN] && tb[IFLA_CAN_TDC_TDCO_MAX]) {
		__u32 *tdco_min = RTA_DATA(tb[IFLA_CAN_TDC_TDCO_MIN]);
		__u32 *tdco_max = RTA_DATA(tb[IFLA_CAN_TDC_TDCO_MAX]);

		can_print_timing_min_max("tdco", " tdco", *tdco_min, *tdco_max);
	}
	if (tb[IFLA_CAN_TDC_TDCF_MIN] && tb[IFLA_CAN_TDC_TDCF_MAX]) {
		__u32 *tdcf_min = RTA_DATA(tb[IFLA_CAN_TDC_TDCF_MIN]);
		__u32 *tdcf_max = RTA_DATA(tb[IFLA_CAN_TDC_TDCF_MAX]);

		can_print_timing_min_max("tdcf", " tdcf", *tdcf_min, *tdcf_max);
	}
	close_json_object();
}

static void can_print_ctrlmode_ext(FILE *f, struct rtattr *ctrlmode_ext_attr,
				   __u32 cm_flags)
{
	struct rtattr *tb[IFLA_CAN_CTRLMODE_MAX + 1];

	parse_rtattr_nested(tb, IFLA_CAN_CTRLMODE_MAX, ctrlmode_ext_attr);
	if (tb[IFLA_CAN_CTRLMODE_SUPPORTED]) {
		__u32 *supported = RTA_DATA(tb[IFLA_CAN_CTRLMODE_SUPPORTED]);

		print_ctrlmode(PRINT_JSON, *supported, "ctrlmode_supported");
		print_ctrlmode(PRINT_JSON, cm_flags & ~*supported, "ctrlmode_static");
	}
}

static void can_print_opt(struct link_util *lu, FILE *f, struct rtattr *tb[])
{
	if (!tb)
		return;

	if (tb[IFLA_CAN_CTRLMODE]) {
		struct can_ctrlmode *cm = RTA_DATA(tb[IFLA_CAN_CTRLMODE]);

		print_ctrlmode(PRINT_ANY, cm->flags, "ctrlmode");
		if (tb[IFLA_CAN_CTRLMODE_EXT])
			can_print_ctrlmode_ext(f, tb[IFLA_CAN_CTRLMODE_EXT],
					       cm->flags);
	}

	if (tb[IFLA_CAN_STATE]) {
		uint32_t state = rta_getattr_u32(tb[IFLA_CAN_STATE]);

		print_string(PRINT_ANY, "state", "state %s ", state < CAN_STATE_MAX ?
			can_state_names[state] : "UNKNOWN");
	}

	if (tb[IFLA_CAN_BERR_COUNTER]) {
		struct can_berr_counter *bc =
			RTA_DATA(tb[IFLA_CAN_BERR_COUNTER]);

		open_json_object("berr_counter");
		print_uint(PRINT_ANY, "tx", "(berr-counter tx %u", bc->txerr);
		print_uint(PRINT_ANY, "rx", " rx %u) ", bc->rxerr);
		close_json_object();
	}

	if (tb[IFLA_CAN_RESTART_MS]) {
		__u32 *restart_ms = RTA_DATA(tb[IFLA_CAN_RESTART_MS]);

		print_uint(PRINT_ANY, "restart_ms", "restart-ms %u ",
			   *restart_ms);
	}

	/* bittiming is irrelevant if fixed bitrate is defined */
	if (tb[IFLA_CAN_BITTIMING] && !tb[IFLA_CAN_BITRATE_CONST]) {
		struct can_bittiming *bt = RTA_DATA(tb[IFLA_CAN_BITTIMING]);
		char sp[6];

		open_json_object("bittiming");
		can_print_nl_indent();
		print_uint(PRINT_ANY, "bitrate", " bitrate %u", bt->bitrate);
		snprintf(sp, sizeof(sp), "%.3f", bt->sample_point / 1000.);
		print_string(PRINT_ANY, "sample_point", " sample-point %s", sp);
		can_print_nl_indent();
		print_uint(PRINT_ANY, "tq", " tq %u", bt->tq);
		print_uint(PRINT_ANY, "prop_seg", " prop-seg %u", bt->prop_seg);
		print_uint(PRINT_ANY, "phase_seg1", " phase-seg1 %u",
			   bt->phase_seg1);
		print_uint(PRINT_ANY, "phase_seg2", " phase-seg2 %u",
			   bt->phase_seg2);
		print_uint(PRINT_ANY, "sjw", " sjw %u", bt->sjw);
		print_uint(PRINT_ANY, "brp", " brp %u", bt->brp);
		close_json_object();
	}

	/* bittiming const is irrelevant if fixed bitrate is defined */
	if (tb[IFLA_CAN_BITTIMING_CONST] && !tb[IFLA_CAN_BITRATE_CONST]) {
		struct can_bittiming_const *btc =
			RTA_DATA(tb[IFLA_CAN_BITTIMING_CONST]);

		open_json_object("bittiming_const");
		can_print_nl_indent();
		print_string(PRINT_ANY, "name", " %s:", btc->name);
		can_print_timing_min_max("tseg1", " tseg1",
					 btc->tseg1_min, btc->tseg1_max);
		can_print_timing_min_max("tseg2", " tseg2",
					 btc->tseg2_min, btc->tseg2_max);
		can_print_timing_min_max("sjw", " sjw", 1, btc->sjw_max);
		can_print_timing_min_max("brp", " brp",
					 btc->brp_min, btc->brp_max);
		print_uint(PRINT_ANY, "brp_inc", " brp_inc %u", btc->brp_inc);
		close_json_object();
	}

	if (tb[IFLA_CAN_BITRATE_CONST]) {
		__u32 *bitrate_const = RTA_DATA(tb[IFLA_CAN_BITRATE_CONST]);
		int bitrate_cnt = RTA_PAYLOAD(tb[IFLA_CAN_BITRATE_CONST]) /
			sizeof(*bitrate_const);
		int i;
		__u32 bitrate = 0;

		if (tb[IFLA_CAN_BITTIMING]) {
			struct can_bittiming *bt =
				RTA_DATA(tb[IFLA_CAN_BITTIMING]);
			bitrate = bt->bitrate;
		}

		can_print_nl_indent();
		print_uint(PRINT_ANY, "bittiming_bitrate", " bitrate %u",
			   bitrate);
		can_print_nl_indent();
		open_json_array(PRINT_ANY, is_json_context() ?
				"bitrate_const" : "    [");
		for (i = 0; i < bitrate_cnt; ++i) {
			/* This will keep lines below 80 signs */
			if (!(i % 6) && i) {
				can_print_nl_indent();
				print_string(PRINT_FP, NULL, "%s", "     ");
			}
			print_uint(PRINT_ANY, NULL,
				   i < bitrate_cnt - 1 ? "%8u, " : "%8u",
				   bitrate_const[i]);
		}
		close_json_array(PRINT_JSON, " ]");
	}

	/* data bittiming is irrelevant if fixed bitrate is defined */
	if (tb[IFLA_CAN_DATA_BITTIMING] && !tb[IFLA_CAN_DATA_BITRATE_CONST]) {
		struct can_bittiming *dbt =
			RTA_DATA(tb[IFLA_CAN_DATA_BITTIMING]);
		char dsp[6];

		open_json_object("data_bittiming");
		can_print_nl_indent();
		print_uint(PRINT_ANY, "bitrate", " dbitrate %u", dbt->bitrate);
		snprintf(dsp, sizeof(dsp), "%.3f", dbt->sample_point / 1000.);
		print_string(PRINT_ANY, "sample_point", " dsample-point %s",
			     dsp);
		can_print_nl_indent();
		print_uint(PRINT_ANY, "tq", " dtq %u", dbt->tq);
		print_uint(PRINT_ANY, "prop_seg", " dprop-seg %u",
			   dbt->prop_seg);
		print_uint(PRINT_ANY, "phase_seg1", " dphase-seg1 %u",
			   dbt->phase_seg1);
		print_uint(PRINT_ANY, "phase_seg2", " dphase-seg2 %u",
			   dbt->phase_seg2);
		print_uint(PRINT_ANY, "sjw", " dsjw %u", dbt->sjw);
		print_uint(PRINT_ANY, "brp", " dbrp %u", dbt->brp);

		if (tb[IFLA_CAN_TDC])
			can_print_tdc_opt(f, tb[IFLA_CAN_TDC]);

		close_json_object();
	}

	/* data bittiming const is irrelevant if fixed bitrate is defined */
	if (tb[IFLA_CAN_DATA_BITTIMING_CONST] &&
	    !tb[IFLA_CAN_DATA_BITRATE_CONST]) {
		struct can_bittiming_const *dbtc =
			RTA_DATA(tb[IFLA_CAN_DATA_BITTIMING_CONST]);

		open_json_object("data_bittiming_const");
		can_print_nl_indent();
		print_string(PRINT_ANY, "name", " %s:", dbtc->name);
		can_print_timing_min_max("tseg1", " dtseg1",
					 dbtc->tseg1_min, dbtc->tseg1_max);
		can_print_timing_min_max("tseg2", " dtseg2",
					 dbtc->tseg2_min, dbtc->tseg2_max);
		can_print_timing_min_max("sjw", " dsjw", 1, dbtc->sjw_max);
		can_print_timing_min_max("brp", " dbrp",
					 dbtc->brp_min, dbtc->brp_max);
		print_uint(PRINT_ANY, "brp_inc", " dbrp_inc %u", dbtc->brp_inc);

		if (tb[IFLA_CAN_TDC])
			can_print_tdc_const_opt(f, tb[IFLA_CAN_TDC]);

		close_json_object();
	}

	if (tb[IFLA_CAN_DATA_BITRATE_CONST]) {
		__u32 *dbitrate_const =
			RTA_DATA(tb[IFLA_CAN_DATA_BITRATE_CONST]);
		int dbitrate_cnt =
			RTA_PAYLOAD(tb[IFLA_CAN_DATA_BITRATE_CONST]) /
			sizeof(*dbitrate_const);
		int i;
		__u32 dbitrate = 0;

		if (tb[IFLA_CAN_DATA_BITTIMING]) {
			struct can_bittiming *dbt =
				RTA_DATA(tb[IFLA_CAN_DATA_BITTIMING]);
			dbitrate = dbt->bitrate;
		}

		can_print_nl_indent();
		print_uint(PRINT_ANY, "data_bittiming_bitrate", " dbitrate %u",
			   dbitrate);
		can_print_nl_indent();
		open_json_array(PRINT_ANY, is_json_context() ?
				"data_bitrate_const" : "    [");
		for (i = 0; i < dbitrate_cnt; ++i) {
			/* This will keep lines below 80 signs */
			if (!(i % 6) && i) {
				can_print_nl_indent();
				print_string(PRINT_FP, NULL, "%s", "     ");
			}
			print_uint(PRINT_ANY, NULL,
				   i < dbitrate_cnt - 1 ? "%8u, " : "%8u",
				   dbitrate_const[i]);
		}
		close_json_array(PRINT_JSON, " ]");
	}

	if (tb[IFLA_CAN_TERMINATION_CONST] && tb[IFLA_CAN_TERMINATION]) {
		__u16 *trm = RTA_DATA(tb[IFLA_CAN_TERMINATION]);
		__u16 *trm_const = RTA_DATA(tb[IFLA_CAN_TERMINATION_CONST]);
		int trm_cnt = RTA_PAYLOAD(tb[IFLA_CAN_TERMINATION_CONST]) /
			sizeof(*trm_const);
		int i;

		can_print_nl_indent();
		print_hu(PRINT_ANY, "termination", " termination %hu [ ", *trm);
		open_json_array(PRINT_JSON, "termination_const");
		for (i = 0; i < trm_cnt; ++i)
			print_hu(PRINT_ANY, NULL,
				 i < trm_cnt - 1 ? "%hu, " : "%hu",
				 trm_const[i]);
		close_json_array(PRINT_JSON, " ]");
	}

	if (tb[IFLA_CAN_CLOCK]) {
		struct can_clock *clock = RTA_DATA(tb[IFLA_CAN_CLOCK]);

		can_print_nl_indent();
		print_uint(PRINT_ANY, "clock", " clock %u ", clock->freq);
	}

}

static void can_print_xstats(struct link_util *lu,
			     FILE *f, struct rtattr *xstats)
{
	struct can_device_stats *stats;

	if (xstats && RTA_PAYLOAD(xstats) == sizeof(*stats)) {
		stats = RTA_DATA(xstats);

		can_print_nl_indent();
		print_string(PRINT_FP, NULL, "%s",
			     " re-started bus-errors arbit-lost error-warn error-pass bus-off");
		can_print_nl_indent();
		print_uint(PRINT_ANY, "restarts", " %-10u", stats->restarts);
		print_uint(PRINT_ANY, "bus_error", " %-10u", stats->bus_error);
		print_uint(PRINT_ANY, "arbitration_lost", " %-10u",
			   stats->arbitration_lost);
		print_uint(PRINT_ANY, "error_warning", " %-10u",
			   stats->error_warning);
		print_uint(PRINT_ANY, "error_passive", " %-10u",
			   stats->error_passive);
		print_uint(PRINT_ANY, "bus_off", " %-10u", stats->bus_off);
	}
}

static void can_print_help(struct link_util *lu, int argc, char **argv, FILE *f)
{
	print_usage(f);
}

struct link_util can_link_util = {
	.id		= "can",
	.maxattr	= IFLA_CAN_MAX,
	.parse_opt	= can_parse_opt,
	.print_opt	= can_print_opt,
	.print_xstats	= can_print_xstats,
	.print_help	= can_print_help,
};
