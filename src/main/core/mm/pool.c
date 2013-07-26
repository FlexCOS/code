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

#include "pool.h"


PRIVATE inline u32 usage_slot_of(u32 e) { return e >> 3; }
PRIVATE inline u8  usage_mark_of(u32 e) { return 1 << (e & 0x7); }

PUBLIC err_t
pool_reset(struct pool *p)
{
	CHECK_PARAM__NOT_NULL (p);
	CHECK_PARAM__NOT_NULL (p->usage);
	CHECK_PARAM__NOT_NULL (p->memb);

	memset(DEST(p->memb), 0x00, p->n * p->msize);
	memset(DEST(p->usage), 0x00, (p->n + 7) >> 3);

	return E_GOOD;
}


PUBLIC void *
pool_get(struct pool *p)
{
	u32 i, used, slot;

	for (i = 0; i < p->n; i++) {
		slot = usage_slot_of(i);
		used = usage_mark_of(i);

		if (p->usage[slot] & used) continue;

		p->usage[slot] ^= used;
		return p->memb + (i * p->msize);
	}

	return NULL;
}

/**
 * Lookup a member of pool p by ID.
 *
 * @return NULL if index is not valid, or it points to an unused member
 */
PUBLIC void *
pool_lookup(const struct pool *p, u32 id)
{
	u32 i, used, slot;

	if ( !id || id > p->n ) return NULL;

	i = id - 1;

	slot = usage_slot_of(i);
	used = usage_mark_of(i);

	if (!(p->usage[slot] & used)) return NULL;

	return p->memb + (i * p->msize);
}

PUBLIC u32
pool_id(const struct pool *p, const void *m)
{
	if ((m < p->memb)                          ||
	    (m > (p->memb + (p->n - 1) * p->msize))||
	    (m - p->memb) % p->msize                )
		return 0;

        /*     ( calculate array index  ) + 1 */
	return ((m - p->memb) / p->msize) + 1;
}

PUBLIC err_t
pool_put(struct pool *p, void *m)
{
	int i, used, slot;
	
	/* is this member m part of the pool? */
	i = pool_id(p, m);
	if (!i) return E_BAD_PARAM;
	i -= 1;

	/* convert array index i to bit position */
	slot = usage_slot_of(i);
	used = usage_mark_of(i);

	/* sanitize member */
	memset(m, 0x00, p->msize);

	if (p->usage[slot] & used)
		p->usage[slot] ^= used;

	return E_GOOD;
}

