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

#include <const.h>
#include <types.h>

#include "flash_layout.h"

#warning "TODO/FIXME: Integer overflows might occur!"

PUBLIC err_t
flash_loc__relocate(struct flash_loc *loc)
{
	const struct flash_cluster *c = &loc->ly->cluster[loc->c];

	/* XXX Sanity check: since relocate must not be used directly this check
	 * might be removed? Alternatively introduce some kind of soundness
	 * check. */
	if ((loc->c >= loc->ly->clusters)
	||  (loc->s >= c->sectors)
	||  (loc->p >= c->pages)
	||  (loc->o >= c->page_size))
	{
		return E_BAD_PARAM;
	}

	loc->abs = c->absolute_offset
	         + loc->s * __sector_size(c)
	         + loc->p * c->page_size
	         + loc->o;

	return E_GOOD;
}

PUBLIC err_t
flash_loc__inc_sector(struct flash_loc *l)
{
	l->o = 0U;
	l->p = 0;
	l->s = (l->s + 1U) % l->ly->cluster[l->c].sectors;
	if (l->s) goto done;
	/* sector overflow -> next cluster */
	l->c = (l->c + 1U) % l->ly->clusters;
	if (l->c) goto done;

	/* end of memory */
	return E_FAILED;
done:
	return flash_loc__relocate(l);
}

PUBLIC err_t
flash_loc__inc_page(struct flash_loc *l)
{
	l->o = 0U;
	l->p = (l->p + 1U) % l->ly->cluster[l->c].pages;
	if (l->p)
		return flash_loc__relocate(l);
	else
		return flash_loc__inc_sector(l);
}

PUBLIC u8
flash_layout__inc_page(const struct flash_layout *ly, struct flash_loc *l)
{
	l->o = 0U;
	/* inc page */
	l->p = (l->p + 1U) % ly->cluster[l->c].pages;

	if (l->p) goto done;
	/* page overflow -> next sector */
	l->s = (l->s + 1U) % ly->cluster[l->c].sectors;

	if (l->s) goto done;
	/* sector overflow -> next cluster */
	l->c = (l->c + 1U) % ly->clusters;
	if (l->c) goto done;

	/* end of memory */
	return 0xff;
done:
	return E_GOOD;
}

/* Given a Flash-Layout definition and an absolute address, this method
 * finds sector and page id this address belongs to.
 *
 * @param[in]  layout - description of flash' sector and page layout.
 * @param[in]    addr - absolute address
 * @param[out] sector - sector id
 * @param[out]   page - page id within sector
 * @param[out] offset - byte offset between start of page and absolute address
 *
 * @return E_GOOD if successful else an error code of enum Err_Code. On error
 * values of sector, page and offset are undefined.
 */
PUBLIC err_t
flash_layout__locate(const struct flash_layout *ly, u32 addr, struct flash_loc *loc)
{
	const struct flash_cluster *cl;
	u32 cl_size;

	if (!flash_layout__contains(ly, addr)) return E_BAD_PARAM;

	/* (1) find cluster, where addr is within its boundaries, and stop. */
	for ((loc->c = 0, loc->s = 0);
	     (loc->c + 1) < ly->clusters && addr >= ly->cluster[(loc->c+1)].absolute_offset;
	     (loc->c++));


	cl = &ly->cluster[loc->c];
	cl_size = __cluster_size(cl);
	/* Sanity check: Probably not needed since address is within total size
	 * range. */
	if (addr > cl->absolute_offset + cl_size)
		return E_ADDRESS;

	loc->abs = addr;
	/* (2) find sector number and page */
	loc->s += (addr / __sector_size(cl));
	loc->o  = addr % __sector_size(cl);
	loc->p  = loc->o / cl->page_size;
	loc->o %= cl->page_size;
	loc->ly = ly;

	return E_GOOD;
}

/**
 *  Extend an existing Flash Layout with a new cluster.
 *
 *  This method assumes a continous address transition from preceding cluster to
 *  this new one, i.e. there is now offset, address gap, whatever.
 *
 *  @param[in,out] l - The Layout object, that should be modified.
 *  @param[in]     s - Number of sectors
 *  @param[in]     p - Number of pages each sector contains.
 *  @param[in]    ps - Page size in bytes.
 *
 *  @return E_GOOD on success,
 */
PUBLIC err_t
flash_layout__append_cluster(struct flash_layout *l, u16 s, u16 p, u16 ps)
{
	u8 c = l->clusters;
	struct flash_cluster *cl = &l->cluster[c];

	if (c >= LENGTH(l->cluster))
		return E_SYSTEM;

	cl->absolute_offset = l->total_size;
	cl->sectors   = s;
	cl->pages     = p;
	cl->page_size = ps;

	l->clusters++;
	/* XXX Integer overflow? */
	l->total_size += __cluster_size(&l->cluster[c]);

	return E_GOOD;
}

/**
 * TODO define contract!
 */
PUBLIC err_t
flash_layout__do_sector_wise(const struct flash_layout *ly,
                             u32 addr_from,
			     u32 addr_to,
			     void *opaque,
			     fp_do_blkwise do_per_sector)
{
	struct flash_loc loc_from;
	struct flash_loc loc_to;
	struct flash_loc loc_eos; /* intemediate location: end of sector */

	err_t  err;

	if ((addr_from >= addr_to)
	||  (flash_layout__locate(ly, addr_from, &loc_from))
	||  (flash_layout__locate(ly, addr_to, &loc_to)))
	{
		return E_BAD_PARAM | E_ADDRESS;
	}

	while (loc_from.s != loc_to.s) {
		flash_loc__sector_end(&loc_from, &loc_eos);

		err = do_per_sector(&loc_from, &loc_eos, opaque);
		if (err) return err;

		/* we have done here move location frame to next sector */
		flash_loc__inc_sector(&loc_from);
	}

	return do_per_sector(&loc_from, &loc_to, opaque);
}

/**
 * TODO define contract!
 */
PUBLIC err_t
flash_layout__do_page_wise(const struct flash_layout *ly,
                             u32 addr_from,
			     u32 addr_to,
			     void *opaque,
			     fp_do_blkwise do_per_sector)
{
	struct flash_loc loc_from;
	struct flash_loc loc_to;
	struct flash_loc loc_eop; /* intemediate location: end of sector */

	err_t  err;

	if ((addr_from >= addr_to)
	||  (flash_layout__locate(ly, addr_from, &loc_from))
	||  (flash_layout__locate(ly, addr_to, &loc_to)))
	{
		return E_BAD_PARAM | E_ADDRESS;
	}

	while (loc_from.s != loc_to.s
	||     loc_from.p != loc_to.p)
	{
		flash_loc__page_end(&loc_from, &loc_eop);

		err = do_per_sector(&loc_from, &loc_eop, opaque);
		if (err) return err;

		/* we have done here move location frame to next sector */
		flash_loc__inc_page(&loc_from);
	}

	return do_per_sector(&loc_from, &loc_to, opaque);
}
