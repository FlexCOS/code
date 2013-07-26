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
 * as3953.c
 *
 *  Created on: Oct 31, 2012
 *      Author: Kristian Beilke
 *      Author: Alexander Münn
 */

/*****************************************************************************/

/***************************** Include Files *********************************/
#include <const.h>
#include <types.h>

#include <string.h>
#include <array.h>

#include "as3953.h"
/************************** Constant Definitions *****************************/


#define AS3953_MASK_MODE     0xE0
#define AS3953_MASK_TRAILER  0x1F

#define AS3953_FIFO_SIZE     32


static u8        _shared_miso_mosi_buffer[AS3953_FIFO_SIZE];
static u8 *const miso = _shared_miso_mosi_buffer;
static u8 *const mosi = _shared_miso_mosi_buffer;

static struct __packed {
	const u8 as3953_mp;
	u8       fifo[AS3953_FIFO_SIZE];
} fifo_send = { .as3953_mp = MP_FIFO_WRITE, .fifo = {0} };


/**
 * This array is used to build ISO 14443-4 blocks.
 *
 * Since we do not want to copy block data on each transmit into an extra buffer
 * dedicated to SPI operations, we point directly to the buffer used for AS3953
 * Fifo load operation only. Shared usage for this buffer is not possible, cause
 * transmitted I-Blocks (indicating chaining) need to be acknowledged by PCD,
 * i.e. if acknowledgement fails, the last I-Block needs to be retransmitted.
 */
PRIVATE struct array output = Array(fifo_send.fifo, sizeof(fifo_send.fifo));

PUBLIC  struct array *const as3953_fifo = &output;


static inline void
clean_miso_mosi(u8 b) {
	memset(_shared_miso_mosi_buffer, 0x00, MIN(AS3953_FIFO_SIZE, b));
}

/**
 * Fetch FIFO into local static buffer.
 *
 * @return Pointer to array containing fifo data. Array length is zero on any
 * failure.
 */
PUBLIC const Array*
as3953_fifo_fetch(u8 max_bytes)
{
	static u8  _buff[AS3953_FIFO_SIZE + 1];
	static Array input = Array(_buff + 1, AS3953_FIFO_SIZE);

	u8 bytes;

	input.length = 0;

	if (max_bytes < AS3953_FIFO_SIZE)
		bytes = max_bytes;
	else if (as3953_register_read(REG_FIFO_STATUS_1, &bytes))
		goto exit;

	if (!bytes) goto exit;

	_buff[0] = MP_FIFO_READ;

	if (as3953_spi_tranfer(_buff, _buff, bytes + 1))
		goto exit;

	input.length = bytes;

exit:
	return &input;
}

PUBLIC err_t
as3953_fifo_prepare_bits(u16 bits)
{
	err_t err;

	if (bits & ~0x1FFF) return E_RANGE;

	if (bits & 0x07)
		bits += 8;

	if ((err = as3953_exec(CMD_CLEAR_FIFO))
	||  (err = as3953_register_write(REG_NUM_TRANS_BYTES_1, (bits >> 8) & 0xFF))
	||  (err = as3953_register_write(REG_NUM_TRANS_BYTES_2, bits & 0xFF)))
	{
		return err;
	}

	return E_GOOD;
}

/**
 *
 */
PUBLIC err_t
as3953_fifo_prepare(u16 length)
{
	return as3953_fifo_prepare_bits(length << 3);
}

/**
 * Load content of global as3953_fifo array into AS3953 FIFO.
 */
PUBLIC err_t
as3953_fifo_commit()
{
 	return as3953_spi_tranfer((u8 *)&fifo_send, NULL, as3953_fifo->length + 1);
}

PUBLIC err_t
as3953_fifo_dpush(const u8 *buff, u8 size)
{
	static u8 block[4];

	if (size > (sizeof(block) - 1)) return E_BAD_PARAM;

	block[0] = MP_FIFO_LOAD;

	memcpy(&block[1], buff, size);

	return as3953_fifo_prepare(size)
	     | as3953_spi_tranfer(block, NULL, size + 1)
	     | as3953_fifo_push();
}

/**
 * Execute a single AS3953 command.
 *
 * Commands are 'set default', 'clear FIFO', 'transmit FIFO' and 'halt'.
 */
PUBLIC err_t
as3953_exec(enum AS3953_Command c)
{
	u8 cmd = MP_CMD | c;

	return as3953_spi_tranfer(&cmd, NULL, 1);
}


/**
 * Read single AS3953 register.
 *
 * @Note: Auto incrementing is not supported.
 */
PUBLIC err_t
as3953_register_read(enum AS3953_Register address, u8 *value)
{
	static u8 buff[2];

	/* Set MODE Pattern */
	buff[0] = MP_REGISTER_READ | (AS3953_MASK_TRAILER & address);
	buff[1] = 0x00;

	if (as3953_spi_tranfer(buff, buff, 2) == E_GOOD) {
		(*value) = buff[1];
		return 0;
	}
	else return 1;
}

/**
 * Write to an AS3953 register.
 *
 * @Note: Auto incrementing is not supported.
 */
PUBLIC err_t
as3953_register_write(enum AS3953_Register address, u8 value)
{
	u8 buff[2];

	if (address == REG_RFID_STATUS   || address == REG_RATS ||
		address == REG_MAIN_IRQ      || address == REG_AUX_IRQ ||
		address == REG_FIFO_STATUS_1 || address == REG_FIFO_STATUS_2)
	{
		return E_ADDRESS;
	}

	/* Set MODE Pattern */
	buff[0] = MP_REGISTER_WRITE | (AS3953_MASK_TRAILER & address);
	buff[1] = value;

	return as3953_spi_tranfer(buff, NULL, 2);
}

/* FIXME this method has not been tested yet */
PUBLIC err_t
as3953_eeprom_write(u8 word, u8 *data, u8 len)
{
	/* write at least one or multiple words */
	if (!len || len % 4 || len > 32) {
		return E_BAD_PARAM;
	}

	/* Do NOT WRITE the LOCK words, but allow write to config-Word and only
	 * this word */
	if ((word  < EE_USER_DATA && word != EE_WORD_CONF) 
	||  (word == EE_WORD_CONF && len  != 4))
	{
		return E_ADDRESS;
	}

	clean_miso_mosi(len + 2);

	/* command starts with 0x40 and word address byte */
	/* address is number of word 0-31 shifted one bit to the left */
	/* followed by 4 data bytes */
	mosi[0] = MP_EEPROM_WRITE;
	mosi[1] = (word & 0x1F) << 1;

	memcpy(&mosi[2], data, len);

	return as3953_spi_tranfer(mosi, NULL, len + 2);
}

/* FIXME this method has not been tested yet */
PUBLIC err_t
as3953_eeprom_read(u8 word_addr, u8* data, u8 len)
{
	int Status;

	if (word_addr > 0x1F)
		return E_ADDRESS;

	/* read at least one or multiple words */
	if (!len || len % 4)
		return E_BAD_PARAM;

	clean_miso_mosi(len + 2);

	/* command starts with 0x7F and word address byte */
	/* address is number of word 0-31 shifted one bit to the left */
	/* answer is 4 data bytes or multiple of it */
	mosi[0] = MP_EEPROM_READ;            /* MODE pattern */
	mosi[1] = word_addr << 1;            /* MODE related data */

	Status = as3953_spi_tranfer(mosi, miso, len + 2);

	/* copy read data anyway */
	memcpy(data, &miso[2], len);

	return Status;
}

/**
 * Returns the UID from EEPROM.
 * value will need to hold 4 bytes
 */
PUBLIC err_t
as3953_getUID(u8 *value)
{
	return as3953_eeprom_read(EE_WORD_UID, value, 4);
}
