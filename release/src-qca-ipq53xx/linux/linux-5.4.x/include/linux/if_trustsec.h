/*
 * trustsec.h
 *	trustsec header definition
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _LINUX_IF_TRUSTSEC_H_
#define _LINUX_IF_TRUSTSEC_H_

#define TRUSTSEC_HLEN	6			/* The additional bytes required by trustsec header. */
#define ETH_P_TRSEC 0x8909			/* Protocol number for trustsec header. */

#define TRUSTSEC_META_HDR_VERSION 0x1		/* Meta header version number. */
#define TRUSTSEC_META_HDR_LEN 0x1		/* trustsec meta header length. */
#define TRUSTSEC_PAYLOAD_LEN 0x0		/* Length of the payload. */
#define TRUSTSEC_PAYLOAD_OPT_TYPE 0x0001	/* Option type field. */

/*
 * trustsec_hdr
 *	Defines trustsec header
 */
struct trustsec_hdr {
	u8 meta_hdr_ver;
	u8 meta_hdr_len;
#if defined(__BIG_ENDIAN_BITFIELD)
	u16 payload_len:3;
	u16 payload_opt_type :13;
#else
	u16 payload_opt_type :13;
	u16 payload_len:3;
#endif
	__be16 sgt;
};

static inline struct trustsec_hdr *trustsec_get_hdr(const struct sk_buff *skb)
{
	return (struct trustsec_hdr *)skb_network_header(skb);
}


/**
 * trustsec_insert_hdr - insert trustsec header
 * @skb: skbuff to insert header
 * @sgt: SGT value
 *
 * Inserts the trustsec header into @skb as part of the payload
 * Returns a trustsec encapsulated skb.
 *
 * Updates skb->protocol to trustsec protocol number
 * Caller must guarantee that the given SKB has enough headroom
 */
static inline void trustsec_insert_hdr(struct sk_buff *skb, __be16 sgt)
{
	struct trustsec_hdr *thdr;

	thdr = (struct trustsec_hdr *)skb_push(skb, TRUSTSEC_HLEN);
	thdr->meta_hdr_ver = TRUSTSEC_META_HDR_VERSION;
	thdr->meta_hdr_len = TRUSTSEC_META_HDR_LEN;
	thdr->payload_len = htons(TRUSTSEC_PAYLOAD_LEN);
	thdr->payload_opt_type = htons(TRUSTSEC_PAYLOAD_OPT_TYPE);
	thdr->sgt = sgt;
	skb->protocol = htons(ETH_P_TRSEC);
}

#endif /* !(_LINUX_IF_TRUSTSEC_H_) */
