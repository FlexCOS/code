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

/*! \file
 *
 *
 */

/* Global configuration and constants */
#include <flxlib.h>
#include <flash_fpga.h>

/* Flash library includes. */
#include <xilflash.h>
#include <xil_types.h>

/* Local directory includes. */
#include "dev.h"
#include "dev_pflash.h"

PRIVATE XFlash FlashInstance;
PRIVATE XFlash *Flash = &FlashInstance;
PRIVATE u8 BlockBuffer[FLASH_BLOCK_SIZE_MAX];

/* ---- Prototypes ---------------------------------------------------------- */

/* implemented in xilflash.c */
PUBLIC extern int XFlashGeometry_ToBlock(
		XFlashGeometry * InstancePtr,
		u32 AbsoluteOffset, u16 *RegionPtr,
		u16 *BlockPtr, u32 *BlockOffsetPtr);
PUBLIC extern int XFlashGeometry_ToAbsolute(
		XFlashGeometry * InstancePtr,
		u16 Region, u16 Block,u32 BlockOffset,
		u32 *AbsoluteOffsetPtr);

PRIVATE err_t uParallelFlashInit();

PRIVATE err_t erase_on_write (XFlash*, u32, size_t, void*);
PRIVATE err_t dev_pflash_read (u32, size_t, u8*);
PRIVATE err_t dev_pflash_write (u32, size_t, u8*);

/*----------------------------------------------------------------------------*/
/*!
 *
 *
 */
PUBLIC err_t
dev_pflash_init( MemDev *FlashDev )
{
	if (!XFlash_IsReady( &FlashInstance ) &&
	    uParallelFlashInit() != E_GOOD)
			return E_HW;

	FlashDev->write = dev_pflash_write;
	FlashDev->read  = dev_pflash_read;

	return E_GOOD;
}
/* ----- end of function: dev_pflash_init() ----- */


/** Initialize a parallel flash module.
 *
 *  @sa config.h
 *  @return XST_SUCCESS or XST_FAILURE on any error
 */
PRIVATE err_t
uParallelFlashInit ()
{
	int err_code;
	/*
	 * Initialize the Flash Library.
	 */
	err_code = XFlash_Initialize(
			&FlashInstance,
			FLASH_BASEADDR,
			FLASH_BUS_WIDTH, 0);

	if (err_code != XST_SUCCESS) {
		return E_HW;
	}

	/*
	 * Reset the Flash Device. This clears the Status registers and puts
	 * the device in Read mode.
	 */
	err_code = XFlash_Reset(&FlashInstance);
	if (err_code != XST_SUCCESS) {
		return E_HW;
	}
	/*
	 * Perform an unlock operation before the erase operation for the Intel
	 * Flash. The erase operation will result in an error if the block is
	 * locked.
	 */
	if ((FlashInstance.CommandSet == XFL_CMDSET_INTEL_STANDARD) ||
	    (FlashInstance.CommandSet == XFL_CMDSET_INTEL_EXTENDED))
	{
		// XXX this makes the whole flash erasable
		err_code = XFlash_Unlock(&FlashInstance, 0x00, FLASH_SIZE);
		if (err_code != XST_SUCCESS) {
			return E_HW;
		}
	}
	return E_GOOD;
}

/**
 * Merge at most 'max_bytes' from buffer src into flash memory.
 * The position is identified by its region, block id and a relative block
 * posititon.
 * After merging all parameter are updated to prepare next block merge.
 *
 * @return error code
 * @sa ERR_CODES
 */
PRIVATE err_t
enqueue_merge_block(XFlash *Flash, u16 *r, u16 *b,
		u32 *pos, u32 *max_bytes, void **src)
{
	err_t err_code;
	buff_t buff = BlockBuffer;

	XFlashGeometry *GeomPtr;
	u32 addr, size, bytes = 0;

	GeomPtr = &(Flash->Geometry);
	size = GeomPtr->EraseRegion[*r].Size;

	err_code = XFlashGeometry_ToAbsolute(GeomPtr, *r, *b, 0, &addr);
	if (err_code) {
		return E_HW;
	}
	/* read whole block into buffer */
	err_code = XFlash_Read(Flash, addr, size, DEST(buff));
	if (err_code) {
		return E_HWR;
	}

	/* merge src bytes into block_buffer */
	bytes = size - *pos;
	bytes = MIN(bytes, *max_bytes);

	memcpy(buff + *pos, *src, bytes);

	err_code = XFlash_Erase(Flash, addr, size);
	if (err_code) {
		return E_INTERN | E_HW;
	}

	err_code = XFlash_Write(Flash, addr, size, SRC(buff));
	if (err_code) {
		return E_INTERN | E_HWW;
	}

	*max_bytes -= bytes;
	*pos = 0;
	*src += bytes;

	XFL_GEOMETRY_INCREMENT(GeomPtr, *r, *b);

	return E_GOOD;
}

PRIVATE err_t
erase_on_write(XFlash *Flash, u32 offset, size_t bytes, void *src)
{
	err_t err_code;
	XFlashGeometry *GeomPtr;
	u32 pos, dummy;

	u16 r, b;
	u16 EndRegion, EndBlock;
	u16 blocks_left;

	if( Flash == NULL ) return E_BAD_PARAM | E_NULL_PTR;
	if( src   == NULL ) return E_BAD_PARAM | E_NULL_PTR;

	if (offset+bytes > FLASH_SIZE) return E_BAD_PARAM | E_ADDRESS;

	/* XFlash Library coding style */
	GeomPtr = &(Flash->Geometry);
	/* calculate block and region boundaries */
	err_code = XFlashGeometry_ToBlock(GeomPtr, offset, &r, &b, &pos);
	if( err_code ) return E_BAD_PARAM | E_ADDRESS;

	err_code = XFlashGeometry_ToBlock(GeomPtr, offset+bytes-1,
			&EndRegion, &EndBlock, &dummy);
	if (err_code) return E_BAD_PARAM | E_ADDRESS;

	blocks_left = XFL_GEOMETRY_BLOCK_DIFF(GeomPtr,
			r, b, EndRegion, EndBlock);

	while (blocks_left && bytes) {
		if( enqueue_merge_block(Flash, &r, &b, &pos, &bytes, &src) ) {
			return E_FAILURE;
		}
		blocks_left--;
	}

	if (blocks_left || bytes) {
		return E_FAILURE;
	}

	return E_GOOD;
}

/*! Use XilFlash library to read from parallel flash module.
 *
 *  \sa MemDevice in somefs/hal.h
 *  \return
 */
PRIVATE err_t
dev_pflash_read( u32 offset, size_t bytes, u8 *dest )
{
	int status;
	u16 r, b;
	u32 dummy;
	XFlashGeometry *GeomPtr = &(FlashInstance.Geometry);

	if (dest == NULL)
		return E_BAD_PARAM | E_NULL_PTR;

	/* check address space */
	if ( XFlashGeometry_ToBlock(GeomPtr, offset, &r, &b, &dummy)        ||
	     XFlashGeometry_ToBlock(GeomPtr, offset+bytes-1, &r, &b, &dummy)||
	     offset + bytes > FLASH_SIZE )
	{
		return E_BAD_PARAM | E_ADDRESS;
	}

	if (Flash == NULL) {
		return E_INTERN | E_NULL_PTR;
	}

	status = XFlash_Read( &FlashInstance, offset, bytes, DEST(dest));
	/* handle failure codes */
	if (status != XST_SUCCESS)
		return E_INTERN | E_HW;
	return E_GOOD;
}

/*! Use XilFlash library to write to parallel flash module.
 *
 *  \param[in]  offset  Relative position to read from.
 *  \param[in]   bytes  Number of bytes to read.
 *  \param[in]     src  Read bytes from this input buffer.
 *
 *  \sa MemDevice in somefs/hal.h
 *  \return
 */
PRIVATE err_t
dev_pflash_write(u32 offset, size_t bytes, u8 *src)
{
	return erase_on_write(&FlashInstance, offset, bytes, SRC(src));
}
