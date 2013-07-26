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
 * as3953_spi.c 
 * 
 *  Created on: Jun 11, 2013 
 *      Author: Kristian Beilke 
 *      Author: Alexander Münn
 **/

#include <const.h>
#include <types.h>
#include <hw.h>

#include <string.h>

#include <array.h>

#include <FreeRTOS.h>

#include <xspi.h>

#include "as3953.h"



PRIVATE XSpi Spi;

/* Dependency of AS3953 API */
PUBLIC err_t as3953_spi_tranfer(u8 *_mosi, u8 *_miso, u32 l)
{
	return XSpi_Transfer(&Spi, _mosi, _miso, l);
}

PUBLIC err_t
as3953_spi_init(u8* config, void(*fp_isr)(void*))
{
	int Status;
	u8  r;
	u8 conf_word[] = { 0x2a, 0x00, 0x20, 0x00 };

	/*
	 * Initialize the SPI driver so that it's ready to use,
	 * specify the device ID that is generated in xparameters.h.
	 */
	if (XSpi_Initialize(&Spi, AS3953_SPI_DEVICE_ID) != XST_SUCCESS)
		return E_HW;

	XSpi_Reset(&Spi);

	/*
	 * Connect the AS3953 antenna to the interrupt subsystem.
	 */
	xPortInstallInterruptHandler(AS3953_INTR_ID, fp_isr, NULL);

	/*
	 * Set the SPI device as a master and in manual slave select mode such
	 * that the slave select signal does not toggle for every byte of a
	 * transfer, this must be done before the slave select is set.
	 */
	Status = XSpi_SetOptions(&Spi, 0x00
			| XSP_MASTER_OPTION
			| XSP_CLK_ACTIVE_LOW_OPTION
			| XSP_MANUAL_SSELECT_OPTION);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	/*
	 * Select the STM flash device on the SPI bus, so that it can be
	 * read and written using the SPI bus.
	 */
	Status = XSpi_SetSlaveSelect(&Spi, 0x01);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Start the SPI driver so that interrupts and the device are enabled.
	 */
	XSpi_Start(&Spi);
	/*
	 * Global interrupts are enabled by default in XSpi_Start.
	 */
	XSpi_IntrGlobalDisable(&Spi);


	//	if (as3953_eeprom_write(EE_WORD_CONF, conf_word, sizeof(conf_word)))
	//		xil_printf("EE writing Configuration Word\n");

	memset(conf_word, 0x00, sizeof(conf_word));

	as3953_register_read(REG_RFID_STATUS, &r);

	if ((r & 0x80) == 0x80)
	{
		xil_printf("Skipping EEPROM config check!\n");
	}
	else
	{
		as3953_eeprom_read(EE_WORD_CONF, conf_word, sizeof(conf_word));
		xil_printf("II [e] EE_WORD_CONF:      0x%02x%02x%02x%02x\n",
				conf_word[0], conf_word[1], conf_word[2], conf_word[3]);

		if (conf_word[0] != config[0] || conf_word[1] != config[1] || conf_word[2] != config[2] || conf_word[3] != config[3])
		{
			as3953_eeprom_write(EE_WORD_CONF, config, sizeof(conf_word));

			memset(conf_word, 0x00, sizeof(conf_word));

			as3953_eeprom_read(EE_WORD_CONF, conf_word, sizeof(conf_word));
		}

		configASSERT(conf_word[0] == config[0]);
		configASSERT(conf_word[1] == config[1]);
		configASSERT(conf_word[2] == config[2]);
		configASSERT(conf_word[3] == config[3]);
	}

	/*
	 * Enable the interrupt for the AS3953 device.
	 */
	vPortEnableInterrupt(AS3953_INTR_ID);

	return E_GOOD;
}

