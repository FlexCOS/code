/*
    FlexCOS - Copyright (C) 2013 AGSI, Department of Computer Science, FU-Berlin

    FOR MORE INFORMATION AND INSTRUCTION PLEASE VISIT
    http://www.inf.fu-berlin.de/groups/ag-si/smart.html


    This file is part of the FlexCOS project.

    FlexCOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 3) as published by the
    Free Software Foundation.

    Some parts of this software are from different projects. These files carry
    a different license in their header.

    FlexCOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
    details. You should have received a copy of the GNU General Public License
    along with FlexCOS; if not it can be viewed here:
    http://www.gnu.org/licenses/gpl-3.0.txt and also obtained by writing to
    AGSI, contact details for whom are available on the FlexCOS WEB site.

*/

#include <flxlib.h>
#include <string.h>
#include <array.h>
#include <io/stream.h>

#include <miracl.h>
#include "ecc.h"
#include "cryptools.h"

#include <stdio.h>

PRIVATE u32  ecies_enc_stream_write(struct stream_out *, u8 *, u32);
PRIVATE void ecies_enc_stream_close(struct stream_out *);

PRIVATE const struct stream_out_ops __ecies_enc_stream_ops = {
		.put  = NULL,
		.write = ecies_enc_stream_write
};

#define AES_BLKSZ 64


struct ecies_enc_stream {
	struct stream_out impl;
	struct hmac       mac;
	aes               ae;
	struct stream_out *target;
	char              blk[AES_BLKSZ];
	u8                lvl;
};

PRIVATE inline struct ecies_enc_stream *
__from_stream_out(struct stream_out *os)
{
	return container_of(os, struct ecies_enc_stream, impl);
}


PUBLIC u32
aes_enc_stream_fetch_from(struct stream_out *os, struct stream_in *is, u32 bytes)
{
	return 0;
}

struct test_struct_ecc_pk_size {
	char buff[sizeof(struct ecc_pk) < 96 ? -1 : 0];
};

PUBLIC struct stream_out *
ecies_enc_stream_create(
		const struct ecc_dom *dom,
		const struct ecc_pk  *pk_to,
		struct stream_out    *target)
{
	struct ecc_pk pk;
	struct ecc_sk sk;

	u8     *mem = (u8 *) &pk;
	struct array  s  = Array(mem, 32);
	struct array  k  = Array(mem + 32, 64);
	struct array  k1 = Array(mem + 32, 32);
	struct array  k2 = Array(mem + 64, 32);

	k1.len = k1.max;
	k2.len = k2.max;

	struct ecies_enc_stream *new = malloc(sizeof(*new));

	if (!new) return NULL;

	new->impl.ops = &__ecies_enc_stream_ops;
	new->lvl = 0;

	/* create ephemeral key */
	ecc_gen(dom, &pk, &sk);

	stream_write_ecc_pk(target, &pk);

	/* calculate a shared key (ECDH-Key-Exchange) */
	ecurve_mult(sk.x, pk_to->Q, pk.Q);
	epoint_norm(pk.Q);
	epoint_get(pk.Q, sk.x, sk.x);

	/* memory of pk is not needed anymore */
	memset(&pk, 0x00, sizeof(pk));

	array_put_big(&s, sk.x);

	/* run KDF => (k1 | k2) */
	kdf_nist2(&s, NULL, &k);
	/* setup aes stream with k1 */
	aes_init(&new->ae, MR_ECB, 32, k.val, NULL);
	/* setup mac stream with k2 */
	hmac_init(&new->mac, &k2);

	return &new->impl;
}

PRIVATE u32
ecies_enc_stream_write(struct stream_out *os, u8 *buff, u32 bytes)
{
	struct ecies_enc_stream *self = __from_stream_out(os);
	u32 passed;

	while (passed < bytes) {
		for (;
		     self->lvl < sizeof(self->blk);
		     self->lvl++, passed++)
		{
			self->blk[self->lvl] = buff[passed];
			hmac_process(&self->mac, buff[passed]);
		}

		if (self->lvl >= sizeof(self->blk)) {
			aes_encrypt(&self->ae, self->blk);
			stream_write(self->target, self->blk, sizeof(self->blk));

			self->lvl = 0;
		}
	}

	return passed;
}

PRIVATE u8
ecies_enc_stream_push()
{
	return 0;
}

PRIVATE void
ecies_enc_stream_close(struct stream_out *os)
{
	u8 __buff[32];
	struct ecies_enc_stream *self = __from_stream_out(os);
	array hash = CArray(__buff);
	/* fill aes block, encrypt */
	if (self->lvl) {
		printf("Ignore incomplete AES block.\n");
	}
	/* clean up aes environment */
	aes_end(&self->ae);

	/* stop MAC processing, write out MAC */
	hmac_hash(&self->mac, &hash);

	stream_write(self->target, hash.val, hash.len);

	return;
}
