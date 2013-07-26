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
// FIXME quick and dirty definition
#define MICRON_FLASH        1U
#define ST_PMOD_FLASH       2U
#include <hw.h>

#include <xintc.h>		/* Interrupt controller device driver */
#include <xspi.h>
#include <xil_exception.h>

#include "flash_layout.h"
#include "flash_ctrl.h"

enum N25Q128_Architecture {
	UNIFORM    = 0x00,
	BOTTOM     = 0x01,
	TOP        = 0x11
};


PRIVATE XSpi Spi;

PRIVATE struct flash_layout   __layout;
PRIVATE struct flash_ctrl_ctx ctrl_ctx = { .layout = &__layout };
PUBLIC  const struct flash_ctrl_ctx *flash_ctrl = &ctrl_ctx;


PUBLIC err_t
__spi_transfer(u8 *mosi, u8 *miso, u32 bytes)
{
	err_t err;

	err = XSpi_Transfer(&Spi, mosi, miso, bytes);

	if(err != XST_SUCCESS)
		return E_INTERN | err;

	return E_GOOD;
}

struct __packed jedec_dev_id {
	u8 manufacturer;
	u8 mem_type;
	u8 mem_size;
};

struct __packed n25q128_dev_id {
	struct jedec_dev_id jedec;
	/* One byte length encoding */
	u8 uid_length;            /* 0x10 */
	/* two bytes Extended Device ID */
	u8 __edid_reserved   : 3;
	u8 __edid_unused     : 3; /* not relevant stuff */
	u8 edid_architecture : 2; /* enum N25Q128_Architecture */;
	/* omitting: */
	/* one byte Extended Device ID */
	/* 14 byte Customized Factory Data */
};

/* Compile time checks */
struct __test_struct_sizes {
	u8 size__jedec_dev_id[sizeof(struct jedec_dev_id) == 3 ? 1 : -1];
	u8 size__n25q128_dev_id[sizeof(struct n25q128_dev_id) == 5 ? 1 : -1];
};

#if SERIAL_FLASH == MICRON_FLASH
PRIVATE err_t
flash_ctrl_init__micron(struct flash_layout *ly)
{
	struct n25q128_dev_id dev_id;

	err_t err;

	err = flash_ctrl_read_id((u8 *) &dev_id, sizeof(dev_id));

	if (err)
		return E_INTERN | err;

	/* Micron 128Mb Flash */
	if (dev_id.jedec.manufacturer != 0x20
	||  dev_id.jedec.mem_type     != 0xBA
	||  dev_id.jedec.mem_size     != 0x18
	||  dev_id.uid_length         != 0x10)
	{
		return E_HW;
	}

	return flash_layout__append_cluster(ly, 256, 256, 256);
}
#endif
/**
 *  Read Identification of SPI Flash and expect values of
 *  16 Mbit ST Serial Flash device.
 *
 *  @param[out] ly - provide layout informations.
 *
 *  @return E_GOOD on success, an error code of enum Err_Codes if either SPI
 *          communication failed or ID data do not agree to expected vaules.
 */
#if SERIAL_FLASH == ST_PMOD_FLASH
PRIVATE err_t
flash_ctrl_init__st_pmod(struct flash_layout *ly)
{
	struct jedec_dev_id dev_id;
	err_t err;

	err = flash_ctrl_read_id((u8 *) &dev_id, sizeof(dev_id));
	if (err) return err;

	/* PMod ST 16Mb Serial Flash */
	if (dev_id.manufacturer != 0x20
	||  dev_id.mem_type     != 0x20
	||  dev_id.mem_size     != 0x15)
	{
		return E_HW;
	}

	return flash_layout__append_cluster(ly, 32, 256, 256);
}
#endif

/**
 *  An abstract initialization method determining (sector) layout properties.
 */
PRIVATE inline err_t
flash_ctrl_init(struct flash_layout *ly)
{
#if     SERIAL_FLASH == MICRON_FLASH
	return flash_ctrl_init__micron(ly);

#elif   SERIAL_FLASH == ST_PMOD_FLASH
	return flash_ctrl_init__st_pmod(ly);
#else
#	error "No Flash Device Macro has been set."
#endif
}



/**
 * Initialize SPI setup for communication with N25Q128 Flash Controller.
 */
PUBLIC err_t
flash_ctrl_spi(void)
{
	err_t err;

	XSpi_Config *ConfigPtr;

	/*
	 * Initialize the SPI driver so that it's ready to use,
	 * specify the device ID that is generated in xparameters.h.
	 */
	ConfigPtr = XSpi_LookupConfig(SFLASH_SPI_DEVICE_ID);
	if (ConfigPtr == NULL) {
		err = E_HW;
		goto init_failed;
	}

	err = XSpi_CfgInitialize(&Spi, ConfigPtr,
				  ConfigPtr->BaseAddress);
	if (err) goto init_failed;

	/*
	 * Set the SPI device as a master and in manual slave select mode such
	 * that the slave select signal does not toggle for every byte of a
	 * transfer, this must be done before the slave select is set.
	 */
	err = XSpi_SetOptions(&Spi, XSP_MASTER_OPTION
	                          | XSP_MANUAL_SSELECT_OPTION);
	if (err) goto init_failed;

	/*
	 * Select the STM flash device on the SPI bus, so that it can be
	 * read and written using the SPI bus.
	 */
	err = XSpi_SetSlaveSelect(&Spi, SFLASH_SPI_SELECT);
	if (err) goto init_failed;

	/*
	 * Start the SPI driver so that interrupts and the device are enabled.
	 */
	err = XSpi_Start(&Spi);
	if (err) goto init_failed;

	/**
	 * Run Xilinx SPI driver in polled mode.
	 */
	XSpi_IntrGlobalDisable(&Spi);

	/**
	 * Read Identification bytes for general verification manufacturer and
	 * architecture properties.
	 */
	return flash_ctrl_init(ctrl_ctx.layout);

init_failed:
	return err;
}
