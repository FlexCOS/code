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

/*! \file A hardware abstraction for operations on arbitrary memory hardware.
 *
 *  \author   Alexander Münn
 *  \version  0.1.2
 *  \date     2012
 */
#pragma once
#ifndef _MEM_DEV_H_
#define _MEM_DEV_H_

/* FIXME this is an evil hack
 *
 */
#define fill_zero(offset, bytes)  write((offset), (bytes), NULL)

struct mem_dev;

typedef struct mem_dev MemDev;

/*!
 * some kind of hardware abstraction!?
 *
 * XXX do we need to introduce an additional context like MemDeviceCtx
 *
 * FIXME this is more like a block device with one byte block size...
 */
struct mem_dev {
	u32  size;
	void *ctx;
	/*!
	 *  \param[in]  offset  Relative position to read from.
	 *  \param[in]   bytes  Number of bytes to read.
	 *  \param[in]     src  Write bytes into this input buffer.
	 */
	err_t (*write)(u32 offset, size_t bytes, buff_t src);
	/*!
	 *  \param[in]  offset  Relative position to read from.
	 *  \param[in]   bytes  Number of bytes to read.
	 *  \param[out]   dest  Copy read bytes into this buffer.
	 */
	err_t (*read)(u32 offset, size_t bytes, buff_t dest);
};

static inline err_t
mdev_write(MemDev *m, u32 off, size_t bytes, u8 *src)
{
	return m->write(off, bytes, src);
}

static inline err_t
mdev_read(MemDev *m, u32 off, size_t bytes, u8 *dest)
{
	return m->read(off, bytes, dest);
}

/* ----- short hand definitions ----- */

#endif
/* ----- end of macro protection _MEM_DEV_H_ ----- */
