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
#include <io/stream.h>

#include <miracl.h>
#include "ecc.h"

PUBLIC void
ecc_pk_init(struct ecc_pk *pk)
{
	memset(pk->__mem, 0x00, sizeof(pk->__mem));
	pk->Y = epoint_init_mem(pk->__mem, 0);
}

PUBLIC void
ecc_sk_init(struct ecc_sk *sk)
{
	memset(sk->__mem, 0x00, sizeof(sk->__mem));
	sk->x = mirvar_mem(sk->__mem, 0);
}

/**
 *  Generate a keypair for given EC domain parameter.
 *
 *  @Note: Assume that dom is active.
 */
PUBLIC err_t
ecc_gen(const struct ecc_dom *dom, struct ecc_pk *pk, struct ecc_sk *sk)
{
	/* be safe and reinitialize key pair memory */
	ecc_pk_init(pk);
	ecc_sk_init(sk);

	bigrand(dom->n, sk->x);
	ecurve_mult(sk->x, dom->G, pk->Y);
	epoint_norm(pk->Y);

	/* Marker is set to 'infinity' if active curve is not set to 'dom' */
	if (point_at_infinity(pk->Y))
		return E_FAILED;

	return E_GOOD;
}

PUBLIC void
ecc_dom_init(struct ecc_dom *c)
{
	memset(c->__bg_mem, 0x00, sizeof(c->__bg_mem));
	memset(c->__pt_mem, 0x00, sizeof(c->__pt_mem));

	c->a = mirvar_mem(c->__bg_mem, 0);
	c->b = mirvar_mem(c->__bg_mem, 1);
	c->p = mirvar_mem(c->__bg_mem, 2);
	c->n = mirvar_mem(c->__bg_mem, 3);

	c->G = epoint_init_mem(c->__pt_mem, 0);
}


PUBLIC err_t
stream_write_ecc_pk(struct stream_out *os, const struct ecc_pk *pk)
{
	char buff[32];
	int  len;
	// TODO bullet proof
	// TODO optional compression

	/* do not use compression */
	stream_put(os, 0x04);
	// FIXME magic number: underlying field size required
	// not sure: is P->X->len too optimistic
	len = big_to_bytes(24, pk->Y->X, buff, TRUE);
	stream_write(os, buff, len);
	len = big_to_bytes(24, pk->Y->Y, buff, TRUE);
	stream_write(os, buff, len);

	// TODO correct return value
	return E_GOOD;
}
