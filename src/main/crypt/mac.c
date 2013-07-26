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
#include <array.h>
#include <io/stream.h>

#include <miracl.h>

#include "cryptools.h"

PRIVATE u32  hmac_stream_out_put(struct stream_out *, u8);
PRIVATE void hmac_stream_out_close(struct stream_out *);

struct hmac_stream_out {
	struct stream_out impl;
	struct hmac       ctx;
	struct stream_out *target;
};

struct stream_out_ops hmac_stream_ops = {
	.put = hmac_stream_out_put,
	.write = NULL,
	.fetch_from = NULL,
	.close = hmac_stream_out_close
};


PRIVATE inline struct hmac_stream_out *
__as_hmac_stream_out(struct stream_out *os)
{
	return container_of(os, struct hmac_stream_out, impl);
}

/**
 *
 */
PUBLIC err_t
hmac_init(struct hmac *hmac, const array *key)
{
	int hlen,b;
	u8 *c;

	/* sha256 */
	hlen = 32;
	b    = 64;

	array_init(&hmac->key,
	           hmac->__key_val,
		   sizeof(hmac->__key_val));

	shs256_init(&hmac->shs);

	if (key->len > b) {
		array_each(&hmac->key, c)
			shs256_process(&hmac->shs, (char) *c);

		shs256_hash(&hmac->shs, (char *) hmac->__key_val);
		hmac->key.len = hlen;
	}
	else {
		array_copy(key, &hmac->key);
	}

	array_fill(&hmac->key, 0x00);

	/* hash inner key pad */
	array_each(&hmac->key, c) shs256_process(&hmac->shs, *c ^ 0x36);

	return E_GOOD;
}

PUBLIC void
hmac_process(struct hmac *hmac, u8 c)
{
	shs256_process(&hmac->shs, c);
}

PUBLIC void
hmac_hash(struct hmac *hmac, array *out)
{
	char *c;
	char *h;

	u8 hlen = 32;

	if (array_bytes_left(out) < hlen) return;

	h = (char *) array_end(out);
	out->len += hlen;

	shs256_hash(&hmac->shs, h);

	array_each(&hmac->key, c) shs256_process(&hmac->shs, *c ^ 0x5c);
	for_each(c, h, hlen)      shs256_process(&hmac->shs, *c);

	shs256_hash(&hmac->shs, h);

	array_clean(&hmac->key);

	return;
}

struct hmac_stream_out *
hmac_stream_out_alloc(void)
{
	struct hmac_stream_out *new;

	new = malloc(sizeof(*new));

	/* TODO errno E_NOMEM */
	if (!new) return NULL;

	new->impl.ops = &hmac_stream_ops;

	return new;
}

void
hmac_stream_out_free(struct hmac_stream_out *os)
{
	if (!os) return;

	/* XXX hm, does the compiler do any optimization here? */
	array_clean(&os->ctx.key);

	free(os);
}

PUBLIC struct stream_out *
hmac_stream_out_create(const array *key, struct stream_out *target)
{
	struct hmac_stream_out *new;

	new = hmac_stream_out_alloc();
	if (!new) return NULL;

	new->target = target;

	if (hmac_init(&new->ctx, key)) {
		hmac_stream_out_free(new);
		return NULL;
	}

	return &new->impl;
}

PRIVATE u32
hmac_stream_out_put(struct stream_out *os, u8 val)
{
	struct hmac_stream_out *hso = __as_hmac_stream_out(os);

	hmac_process(&hso->ctx, val);

	return stream_put(hso->target, val);
}

PRIVATE void
hmac_stream_out_close(struct stream_out *os)
{
	struct hmac_stream_out *hso = __as_hmac_stream_out(os);
	u8 __mem_hash[32];
	array mac = CArray(__mem_hash);

	hmac_hash(&hso->ctx, &mac);

	stream_write(hso->target, mac.val, mac.len);

	return;
}
