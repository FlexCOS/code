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

/* 
 * octets are very similar to our array representation...
 */

typedef unsigned int oltype;

typedef struct octet {
	oltype len;
	oltype max;
	u8    *val;
} octet;

#define Octet(arr) { .len = 0, .max = sizeof(arr), .val = (arr) }
#define octet_each(oct, c) \
	for ((c) = (oct)->val; (c) < (oct)->val + (oct)->len; (c)++)

PRIVATE inline oltype
octet_bytes_left(const octet *o)
{
	return o->max - o->len;
}

PRIVATE inline bool
octet_has_bytes_free(const octet *o, oltype b)
{
	return b < (o->max - o->len);
}

PRIVATE inline unsigned char *
octet_end(octet *o)
{
	return o->val + o->len;
}

PRIVATE inline u8
octet_put(octet *o, u8 c)
{
	if (o->len < o->max) {
		o->val[o->len++] = c;
		return 1;
	}

	return 0;
}

PRIVATE inline u8
octet_push(octet *o, u8 c)
{
	return octet_put(o, c);
}

PRIVATE inline u8
octet_pop(octet *o, u8 *c)
{
	if (o->len) {
		*c = o->val[o->len--];
		return 1;
	}
	return 0;
}

static inline void
octet_reset(octet *o)
{
	o->len = 0;
}

static inline void
octet_clear(octet *o)
{
	memset(o->val, 0x00, o->max);
	o->len = 0;
}

PUBLIC void   octet_release(octet *);
PUBLIC oltype octet_fill(octet *, u8);
PUBLIC oltype octet_fill_random(octet *, oltype);

/**
 *  @return number of octets that have been appended.
 */
PRIVATE oltype
octet_append(octet *to, const char *data, oltype bytes)
{
	bytes = MIN(bytes, octet_bytes_left(to));
	memcpy(to->val, data, bytes);
	to->len += bytes;

	return bytes;
}


