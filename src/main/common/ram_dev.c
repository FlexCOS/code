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

#include <string.h>

#include <io/dev.h>
#include <io/stream.h>

#include "ram_dev.h"

#define STUB_MEMDEV_SIZE (4 * 64 * 1024)

PRIVATE u8 storage[STUB_MEMDEV_SIZE];
PRIVATE const u32 mem_size = STUB_MEMDEV_SIZE;

PRIVATE err_t ram_dev_read (u32, size_t, buff_t);
PRIVATE err_t ram_dev_write(u32, size_t, buff_t);

PUBLIC err_t
ram_dev_init(MemDev *mdev)
{
	if (mdev == NULL)
		return (E_BAD_PARAM | E_NULL_PTR);

	memset (storage, 0x00, mem_size);

	mdev->size  = mem_size;
	mdev->read  = ram_dev_read;
	mdev->write = ram_dev_write;

	return E_GOOD;
}

PUBLIC err_t
ram_dev_release(MemDev *mdev)
{
	if( mdev->read  == ram_dev_read &&
	    mdev->write == ram_dev_write)
	{
		mdev->read = NULL;
		mdev->write = NULL;

		return E_GOOD;
	}
	return E_BAD_PARAM;
}

/* @implements method read(*) of struct mem_dev */
PRIVATE err_t
ram_dev_read(u32 offset, size_t bytes, buff_t buff)
{
	if ( (offset + bytes) > mem_size)
		return (E_BAD_PARAM | E_ADDRESS);
	if (buff == NULL)
		return (E_BAD_PARAM | E_NULL_PTR);
	if (bytes == 0)
		return E_GOOD;

	memcpy(buff, storage+offset, bytes);


	return E_GOOD;
}

// @implements method write(*) of struct mem_dev
PRIVATE err_t
ram_dev_write(u32 offset, size_t bytes, buff_t buff)
{
	if ( (offset + bytes) > mem_size)
		return (E_BAD_PARAM | E_ADDRESS);
	if (bytes == 0)
		return E_GOOD;

#       warning Work around - Accept NULL pointer

	if (buff)
		memcpy(storage + offset, SRC(buff), bytes);
	else
		memset(storage + offset, 0x00, bytes);

	return E_GOOD;
}
