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
#include <array.h>

#include "flash_layout.h"
#include "flash_ctrl.h"

/*
 * Byte Positions.
 */
#define BYTE1				0 /* Byte 1 position */
#define BYTE2				1 /* Byte 2 position */
#define BYTE3				2 /* Byte 3 position */
#define BYTE4				3 /* Byte 4 position */
#define BYTE5				4 /* Byte 5 position */

extern err_t __spi_transfer(u8 *, u8 *, u32);

enum Generic_Cmd {
	CMD_WRITE                       = 0x02,
	CMD_READ                        = 0x03,
	CMD_WRITE_DISABLE               = 0x04,
	CMD_READ_STATUS                 = 0x05,
	CMD_READ_ID                     = 0x9F,
	CMD_WRITE_ENABLE                = 0x06,
	CMD_BULK_ERASE                  = 0xc7,
	CMD_SECTOR_ERASE                = 0xD8,
};

/**
 * This definitions specify the EXTRA bytes in each of the command
 * transactions. This count includes Command byte, address bytes and any
 * don't care bytes needed.
 */
#define CMD_LENGTH_RW                  4 /* Read/Write extra bytes */

/*
 * Flash not busy mask in the status register of the flash device.
 */
#define FLASH_WRITE_IN_PROGRESS         0x01

/*
 * The instances to support the device drivers are global such that they
 * are initialized to zero each time the program runs. They could be local
 * but should at least be static so they are zeroed.
 */
PRIVATE err_t flash_ctrl_get_status(u8 *);

/*
 * Buffers used during Read/Write transactions.
 */
#define FLASH_MAX_PAGE_SIZE 256
PRIVATE u8 __shared_miso_mosi_buffer[FLASH_MAX_PAGE_SIZE + CMD_LENGTH_RW];
PRIVATE u8 *const WriteBuffer = __shared_miso_mosi_buffer;

/**
 * This function waits till the serial Flash is ready to accept next command.
 *
 * @param	None
 *
 * @return	E_GOOD if successful else an error code of enum Err_Codes.
 *
 * @note	This function reads the status register of the Buffer and waits
 *.		till the WIP bit of the status register becomes 0.
 */
PUBLIC err_t flash_ctrl_wait(void)
{
	err_t err;
	u8    status;

	/**
	 * Poll the Status Register until READY_BIT has been set.
	 */
	do {
		if ((err = flash_ctrl_get_status(&status)))
			return err;

	} while (status & FLASH_WRITE_IN_PROGRESS);

	return E_GOOD;
}

/**
 * Read Identification of Flash Device.
 */
PUBLIC err_t
flash_ctrl_read_id(u8 *data, u8 size)
{
	err_t err;
	WriteBuffer[0] = CMD_READ_ID;

	err = __spi_transfer(WriteBuffer, WriteBuffer, size + 1);
	if (err) return err;

	memcpy(data, WriteBuffer + 1, size);

	return E_GOOD;
}


PUBLIC err_t
flash_ctrl_page_prog(u32 addr, const u8 *data, u16 bytes)
{
	struct array mosi = Array(WriteBuffer, 260);

	if (addr >> 24) return E_BAD_PARAM | E_ADDRESS;

	array_truncate(&mosi);

	array_put(&mosi, CMD_WRITE);
	array_put(&mosi, (addr >> 16));
	array_put(&mosi, (addr >> 8));
	array_put(&mosi, (addr));

	array_append(&mosi, data, bytes);

	return __spi_transfer(mosi.v, NULL, mosi.length);
}

/**
 * This function reads the data from generic Serial Flash Memory.
 */
PUBLIC err_t
flash_ctrl_read(u32 addr, u8 *data, u32 bytes)
{
	err_t err;
	static u8 miso[2 * CMD_LENGTH_RW];

	if (bytes > CMD_LENGTH_RW) {
		addr += CMD_LENGTH_RW;
		WriteBuffer[BYTE1] = CMD_READ;
		WriteBuffer[BYTE2] = (u8) (addr >> 16);
		WriteBuffer[BYTE3] = (u8) (addr >> 8);
		WriteBuffer[BYTE4] = (u8)  addr;

		err = __spi_transfer(WriteBuffer, data, bytes);

		if (err) return E_HWR;
		addr -= CMD_LENGTH_RW;
	}

	bytes = MIN(CMD_LENGTH_RW, bytes);

	WriteBuffer[BYTE1] = CMD_READ;
	WriteBuffer[BYTE2] = (u8) (addr >> 16);
	WriteBuffer[BYTE3] = (u8) (addr >> 8);
	WriteBuffer[BYTE4] = (u8)  addr;

	err = __spi_transfer(WriteBuffer, miso, CMD_LENGTH_RW + bytes);

	if (err) return E_HWR;

	memcpy(data, miso + CMD_LENGTH_RW, bytes);

	return E_GOOD;
}

/**
 * Use data buffer directly as miso/mosi channel, i.e. there is no additional
 * memcpy invocation as in plain flash_ctrl_read().
 *
 * This method writes four byte SPI command payload into memory before data
 * buffer. Although overwritten data gets restored at the end there is a guard
 * protecting against incorrect invocation: These four bytes must be set to
 * value of FLASH_CTRL_CMD_RESERVED macro.
 */
PUBLIC err_t
flash_ctrl_read_passthrough(u32 addr, u8 *data, u32 bytes)
{
	err_t rval;
	u32 backup;
	u8  *moso = data - sizeof(backup);

	backup = *((u32 *) moso);

	moso[BYTE1] = CMD_READ;
	moso[BYTE2] = (u8) (addr >> 16);
	moso[BYTE3] = (u8) (addr >> 8);
	moso[BYTE4] = (u8)  addr;

	if (backup != FLASH_CTRL_CMD_RESERVED)
		return E_BAD_PARAM | E_ADDRESS;

	rval = __spi_transfer(moso, moso, bytes + sizeof(backup));

	*(u32 *) moso = backup;

	return rval;
}

/**
 * This function erases the entire contents of the Serial Flash device.
 */
PUBLIC err_t
flash_ctrl_bulk_erase(void)
{
	u8 mosi = CMD_BULK_ERASE;

	return __spi_transfer(&mosi, NULL, sizeof(mosi));
}

PUBLIC err_t
flash_ctrl_write_enable(void)
{
	u8 mosi = CMD_WRITE_ENABLE;

	flash_ctrl_wait();

	return __spi_transfer(&mosi, NULL, sizeof(mosi));
}

/*****************************************************************************/
/**
*
* This function erases the contents of the specified Sector in the STM Serial
* Flash device.
*
* @param[in]    addr - the address within a sector of the Buffer, which is to
*		be erased.
*
* @return	E_GOOD if successful else some error code of enum Err_Code.
******************************************************************************/
PUBLIC err_t
flash_ctrl_sector_erase(u32 addr)
{
	u8 mosi[4];

	// if (!__is_sector_address(addr)) return E_BAD_PARAM;

	/*
	 * Prepare the WriteBuffer.
	 */
	mosi[BYTE1] = CMD_SECTOR_ERASE;
	mosi[BYTE2] = (u8) (addr >> 16);
	mosi[BYTE3] = (u8) (addr >> 8);
	mosi[BYTE4] = (u8) (addr);

	return __spi_transfer(mosi, NULL, sizeof(mosi));
}

/**
 * This function reads the Status register of the STM Flash.
 *
 * @return	E_GOOD or an enum Err_Code.
 */
PRIVATE err_t
flash_ctrl_get_status(u8 *status)
{
	err_t err;
	u8 misi[2];
	/*
	 * Prepare the Write Buffer.
	 */
	misi[BYTE1] = CMD_READ_STATUS;

	err = __spi_transfer(misi, misi, sizeof(misi));

	if (err) return err;

	*status = misi[1];

	return E_GOOD;
}
