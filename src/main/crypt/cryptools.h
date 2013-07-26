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
struct array;

struct hmac {
	/* XXX magic number is refered to hash block size. */
	u8     __key_val[64];
	array  key;
	/* TODO we might want to opaque used SHA implementation */
	sha256 shs;
};

err_t hmac_init(struct hmac *, const array *);
void  hmac_process(struct hmac *, u8);
void  hmac_hash(struct hmac *, array *);

err_t kdf_nist2(const array *, const array *, array *);

/* extend array handling */
u16 array_put_big(array *, const big);

struct stream_out *hmac_stream_out_create(const array *, struct stream_out *);
