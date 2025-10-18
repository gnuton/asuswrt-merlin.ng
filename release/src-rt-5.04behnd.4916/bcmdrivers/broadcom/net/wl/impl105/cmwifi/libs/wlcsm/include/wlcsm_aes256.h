/*
*   Byte-oriented AES-256 implementation.
*   All lookup tables replaced with 'on the fly' calculations.
*
*   Copyright (c) 2007-2009 Ilya O. Levin, http://www.literatecode.com
*   Other contributors: Hal Finney
*
*   Permission to use, copy, modify, and distribute this software for any
*   purpose with or without fee is hereby granted, provided that the above
*   copyright notice and this permission notice appear in all copies.
*
*   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
*   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
*   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
*   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
*   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
*   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
*   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
/*
 * <<Broadcom-WL-IPTag/Open>>
 */
#ifndef __WLCSM_AES256_H__
#define __WLCSM_AES256_H__

#define AES256_BLOCK_SIZE 16
#define AES256_KEY_SIZE 32

typedef struct {
    unsigned char key[AES256_KEY_SIZE];
    unsigned char enckey[AES256_KEY_SIZE];
    unsigned char deckey[AES256_KEY_SIZE];
} aes256_context;

void aes256_init(aes256_context *, unsigned char * /* key */);
void aes256_done(aes256_context *);
void aes256_encrypt_ecb(aes256_context *, unsigned char * /* plaintext */);
void aes256_decrypt_ecb(aes256_context *, unsigned char * /* cipertext */);

#endif // __AES256_H__
