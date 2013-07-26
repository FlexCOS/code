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

#pragma once

struct stream_out;

struct ecc_dom;
struct ecc_pk;
struct ecc_sk;

typedef struct bigtype *big;

struct ecc_dom {
	char __pt_mem[MR_ECP_RESERVE(1)];
	char __bg_mem[MR_BIG_RESERVE(4)];
	u16    bits;
	u16    fsize;
	big    a;  /* curve coeff */
	big    b;
	big    p;  /* prime field */
	big    n;  /* order of G */
	epoint *G; /* kernel */
	/* missing: cofactor */
};

struct ecc_pk {
	char __mem[MR_ECP_RESERVE(1)];
	union {
		epoint *Y __deprecated;
		epoint *Q;
	};
};

struct ecc_sk {
	char __mem[MR_BIG_RESERVE(1)];
	big  x;
};

struct elg_signature {
	struct array *r;
	struct array *s;
};

err_t ecc_gen(const struct ecc_dom *, struct ecc_pk *, struct ecc_sk *);
void  ecc_pk_init(struct ecc_pk *);
void  ecc_sk_init(struct ecc_sk *);
void  ecc_dom_init(struct ecc_dom *);

err_t ecc_elg_sign_start(const struct array *,
                         const struct ecc_sk *,
			 const struct ecc_pk *,
			 struct elg_signature *);

err_t ec2p_elg_sign_complete(void);

struct stream_out *
ecies_enc_stream_create(
		const struct ecc_dom *,
		const struct ecc_pk *,
		struct stream_out *);

/* Extend Streaming API */
/* TODO consider compression flag */
err_t stream_write_ecc_pk(struct stream_out *, const struct ecc_pk *);

