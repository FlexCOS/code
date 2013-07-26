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
 *
 *  Created on: Oct 18, 2012
 *      Author: Kristian Beilke
 *      Author: Alexander Münn
 */
#include <const.h>
#include <types.h>
#include <hw.h>

#include <string.h>
#include <assert.h>

#include <io/dev.h>

#include <array.h>

#include "flash_ctrl.h"
#include "flash_layout.h"
#include "flash_dev_simple.h"

PRIVATE err_t flash_dev_erase(u32, u32);

/* FIXME Better name. Documentation. */
PRIVATE const struct flash_layout *__layout;

/* Special memory organization to fullfill contract of flash_ctrl_*_passthrough
 * methods. */
PRIVATE struct {
	const u32 flash_ctrl_reserved;
	u8 data[256*256];
} flash_ctrl_pass_through_memory = {
	.flash_ctrl_reserved = FLASH_CTRL_CMD_RESERVED,
	.data = {0}
};
PRIVATE u8 *const sector_backup = flash_ctrl_pass_through_memory.data;


/**
 * Initialization
 */
PUBLIC err_t
flash_dev_simple(struct mem_dev *dev)
{
	err_t err;

	err = flash_ctrl_spi();
	if (err) return E_SYSTEM | err;
	__layout = flash_ctrl->layout;

	dev->read  = flash_dev_read;
	dev->write = flash_dev_write;
	dev->size  = __layout->total_size;

	return E_GOOD;
}

struct write_ctx {
	const u8 *buff;
	u32 bytes_left;
};

/**
 *
 */
PRIVATE err_t
__write_to_page(const struct flash_loc *loc_from,
               const struct flash_loc *loc_to,
	       void *opaque)
{
	struct write_ctx *ctx  = (struct write_ctx *) opaque;
	u32 bytes;

	assert(loc_from->p == loc_to->p);
	assert(loc_from->o <= loc_to->o);
	assert(ctx->bytes_left != 0);

	bytes = loc_to->abs - loc_from->abs + 1;

	if (flash_ctrl_wait()
	||  flash_ctrl_write_enable()
	||  flash_ctrl_page_prog(loc_from->abs, ctx->buff, bytes))
	{
		return E_HWW;
	}

	ctx->bytes_left -= bytes;
	ctx->buff       += bytes;

	return E_GOOD;
}

PRIVATE err_t
flash_dev_write_xor(u32 addr, const u8 *data, u32 bytes)
{
	struct write_ctx ctx = {
		.buff = data,
		.bytes_left = bytes
	};

	return flash_layout__do_page_wise(
			__layout,
			addr,
			addr + bytes - 1,
			&ctx,
			__write_to_page);
}

/**
 * Erase all bits within parameter range 'loc_from' and 'loc_to'.
 *
 * Both location are supposed to be in the same sector, since invocation of this
 * method should be handled by 'flash_layout__do_sector_wise'.
 *
 * @Implements fp_do_blkwise in flash_layout.h
 */
PRIVATE err_t
backup_erase_restore(const struct flash_loc *loc_from,
                     const struct flash_loc *loc_to,
		     void *ctx)
{
	/* container for 'real' sector start and end address */
	struct flash_loc loc_ss;
	struct flash_loc loc_se;
	u32 bytes;
	u16 pos;
	err_t err;

	assert(loc_from->s   == loc_to->s);
	assert(loc_from->abs <= loc_to->abs);

	flash_loc__sector_start(loc_from, &loc_ss);
	flash_loc__sector_end(loc_to, &loc_se);

	bytes = flash_loc__sector_size(&loc_ss);
	/*  */
	if ((err = flash_ctrl_wait())
	||  (err = flash_ctrl_read_passthrough(loc_ss.abs, sector_backup, bytes))
	||  (err = flash_ctrl_write_enable())
	||  (err = flash_ctrl_sector_erase(loc_ss.abs)))
	{
		goto failed;
	}

	/* Restore sector data from first sector byte to (excluding) loc_from */
	if (loc_from->abs > loc_ss.abs) {
		err = flash_dev_write_xor(
				loc_ss.abs,
				sector_backup,
				loc_from->abs - loc_ss.abs);
		if (err) goto failed;
	}

	/* Restore sector data from (excluding) loc_to to last sector byte */
	if (loc_to->abs < loc_se.abs) {
		pos = flash_loc__sector_addr(loc_to) + 1;
		err = flash_dev_write_xor(
				loc_to->abs + 1,
				sector_backup + pos,
				bytes - pos);
		if (err) goto failed;
	}

	return E_GOOD;
failed:
	return E_INTERN | err;
}


/**
 *
 */
PRIVATE err_t
flash_dev_erase(u32 blk_start, size_t blk_bytes)
{
	/* XXX Nothing to do here. Is it good or bad? */
	if (!blk_bytes) return E_BAD_PARAM;

	return flash_layout__do_sector_wise(
			__layout,
			blk_start,
			blk_start + blk_bytes - 1,
			NULL,
			backup_erase_restore);
}

/**
 * @Implements mem_dev->write
 */
PUBLIC err_t
flash_dev_write(u32 addr, size_t bytes, const buff8_t src)
{
	err_t err;

	CHECK_PARAM__NOT_NULL(src);

	if (!bytes) return E_GOOD;

	if (!flash_layout__contains(__layout, addr)
	||  !flash_layout__contains(__layout, addr + bytes - 1)
	||  (addr > (addr + bytes)))
	{
		return E_BAD_PARAM | E_ADDRESS;
	}

	if ((err = flash_dev_erase(addr, bytes))
	||  (err = flash_dev_write_xor(addr, src, bytes)))
	{
		return err;
	}

	return E_GOOD;
}

/**
 * Random Read of Flash Memory.
 *
 * @Implements mem_dev->read
 */
PUBLIC err_t
flash_dev_read(u32 addr, size_t bytes, buff_t dest)
{
	CHECK_PARAM__NOT_NULL(dest);

	if (!flash_layout__contains(__layout, addr)
	||  !flash_layout__contains(__layout, addr + bytes - 1)
	||  (addr > (addr + bytes)))
	{
		return E_BAD_PARAM | E_ADDRESS;
	}

	flash_ctrl_wait();

	return flash_ctrl_read(addr, dest, bytes);
}
