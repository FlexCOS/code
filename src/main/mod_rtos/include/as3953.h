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

struct array;

enum AS3953_Register {
	REG_IO_CONF		= 0x00,
	REG_MODE_DEFINE		= 0x01,
	REG_BIT_RATE		= 0x02,
	REG_RFID_STATUS		= 0x04,
	REG_RATS		= 0x05,
	REG_MASK_MAIN_IRQ	= 0x08,
	REG_MASK_AUX_IRQ	= 0x09,
	REG_MAIN_IRQ		= 0x0A,
	REG_AUX_IRQ		= 0x0B,
	REG_FIFO_STATUS_1	= 0x0C,
	REG_FIFO_STATUS_2	= 0x0D,
	REG_NUM_TRANS_BYTES_1	= 0x10,
	REG_NUM_TRANS_BYTES_2	= 0x11,
};
enum AS3953_Register_Main {
	MAIN_IRQ_POWER_UP	= 0x80,
	MAIN_IRQ_ACTIVE 	= 0x40,
	MAIN_IRQ_WAKE_UP	= 0x20,
	MAIN_IRQ_START_RX	= 0x10,
	MAIN_IRQ_END_RX		= 0x08,
	MAIN_IRQ_END_TX		= 0x04,
	MAIN_IRQ_FIFO_WATER	= 0x02,
	MAIN_IRQ_AUX		= 0x01,
};
enum AS3953_Register_Aux {
	AUX_IRQ_DESELECT      		= 0x80,
	AUX_IRQ_FRAME_ERR 	        = 0x40,
	AUX_IRQ_PARITY_ERR     	        = 0x20,
	AUX_IRQ_CRC_ERR        	        = 0x10,
	AUX_IRQ_FIFO_ERR       	        = 0x08,
	AUX_IRQ_EEPROM_WRITE_OK        	= 0x04,
	AUX_IRQ_EEPROM_WRITE_FAIL     	= 0x02,
	AUX_IRQ_EEPROM_ACCES_INTR      	= 0x01,
};
enum AS3953_Register_Fifo2 {
	FIFO_UNDERFLOW                 = 0x40,
	FIFO_OVERFLOW                  = 0x20,
	FIFO_LB_INCOMPLETE             = 0x10,
	FIFO_LMSB_VALID                = 0x0E,
	FIFO_NO_PARITY                 = 0x01,
};

enum AS3953_E2_Words {
	EE_WORD_UID             = 0,
	EE_WORD_FAB             = 1,
	EE_WORD_CONF            = 2,
	EE_WORD_WLOCK           = 3,
	EE_WORD_RLOCK           = 4,
	EE_USER_DATA            = 5,
};

enum AS3953_Mode_Pattern {
	MP_REGISTER_WRITE       = 0x00,
	MP_REGISTER_READ        = 0x20,
	MP_EEPROM_WRITE         = 0x40,
	MP_EEPROM_READ          = 0x7F,
	MP_FIFO_READ            = 0xBF,
	MP_FIFO_WRITE           = 0x80,
	MP_CMD                  = 0xC0,
	MP_FIFO_LOAD            = MP_FIFO_WRITE,
};

enum AS3953_Command {
	CMD_SET_DEFAULT         = 0xC2,
	CMD_CLEAR_FIFO          = 0xC4,
	CMD_TRANSMIT            = 0xC8,
	CMD_HALT                = 0xD0,
};

/* provide read only access to fifo output data */
extern struct array *const as3953_fifo;

err_t as3953_register_read(enum AS3953_Register address, u8 *value);
err_t as3953_register_write(enum AS3953_Register address, u8 value);
err_t as3953_exec(enum AS3953_Command c);
err_t as3953_eeprom_read(u8 word_addr, u8* data, u8 len);
err_t as3953_eeprom_write(u8 word, u8 *data, u8 len);
err_t as3953_getUID(u8 *value);

const struct array* as3953_fifo_fetch(u8);

static inline size_t as3953_fifo_add(const u8 *, u8);
static inline err_t  as3953_fifo_push();
err_t as3953_fifo_commit();
err_t as3953_fifo_prepare(u16);
err_t as3953_fifo_prepare_bits(u16);
err_t as3953_fifo_dpush(const u8 *, u8);

/* Extern dependencies */
err_t as3953_spi_tranfer(u8 *_mosi, u8 *_miso, u32 l);
err_t as3953_spi_init(u8* config, void(*fp_isr)(void*));

static inline void as3953_fifo_reset() {
	array_reset(as3953_fifo);
	return;
}

static inline size_t as3953_fifo_add(const u8 *b, u8 l) {
	return array_append(as3953_fifo, b, l);
}

static inline err_t as3953_fifo_push() {
	return as3953_exec(CMD_TRANSMIT);
}

