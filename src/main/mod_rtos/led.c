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

/*-----------------------------------------------------------
 * Simple digital IO routines.
 *-----------------------------------------------------------*/
#include <const.h>
#include <types.h>
#include <hw.h>

/* Kernel includes. */
#include "FreeRTOS.h"

/* Application includes. */
#include "led.h"

/* Library includes. */
#include "xgpio.h"

/* A hardware specific constant required to use the Xilinx driver library. */
static const unsigned portBASE_TYPE uxGPIOOutputChannel = 1UL;

/* The current state of the output port. */
static unsigned char ucGPIOState = 0U;

/* Structure that hold the state of the ouptut peripheral used by this demo.
This is used by the Xilinx peripheral driver API functions. */
static XGpio xOutputGPIOInstance;

/*
 * Setup the IO for the LED outputs.
 */
err_t 
led_init(void)
{
	portBASE_TYPE xStatus;
	const u8 ucSetToOutput = 0U;

	/* Initialise the GPIO for the LEDs. */
	xStatus = XGpio_Initialize(
			&xOutputGPIOInstance, 
			LED_GPIO_DEVICE_ID);

	if (xStatus == XST_SUCCESS) {
		/* All bits on this channel are going to be outputs (LEDs). */
		XGpio_SetDataDirection(
				&xOutputGPIOInstance, 
				uxGPIOOutputChannel, 
				ucSetToOutput);

		/* Start with all LEDs off. */
		ucGPIOState = 0U;
		XGpio_DiscreteWrite( &xOutputGPIOInstance, uxGPIOOutputChannel, ucGPIOState );
	}
	
	/* FIXME return valid FlexCOS error code */
	return xStatus;
}

void
led_set_masked(unsigned portBASE_TYPE uxLEDs, enum LED_STATUS leds_on)
{
	u8 ucLEDs = (0xFF >> (8 - LED_CNT_MAX));

	ucLEDs &= uxLEDs;

	portENTER_CRITICAL(); 
	{
		if (leds_on)
			ucGPIOState |= ucLEDs;
		else
			ucGPIOState &= ~ucLEDs;

		XGpio_DiscreteWrite(&xOutputGPIOInstance,
				uxGPIOOutputChannel,
				ucGPIOState );
	}
	portEXIT_CRITICAL();
}

void
led_set_single(unsigned portBASE_TYPE uxLED, enum LED_STATUS led_on) 
{
	u8 ucLED = 1U;

	/* Only attempt to set the LED if it is in range. */
	if (uxLED < LED_CNT_MAX) {
		ucLED <<= (unsigned char) uxLED;

		portENTER_CRITICAL(); 
		{
			if (led_on)
				ucGPIOState |= ucLED;
			else
				ucGPIOState &= ~ucLED;

			XGpio_DiscreteWrite(&xOutputGPIOInstance,
					uxGPIOOutputChannel,
					ucGPIOState );
		}
		portEXIT_CRITICAL();
	}
}

void
vLedToggle(unsigned portBASE_TYPE uxLED)
{
	u8 ucLED = 1U;

	/* Only attempt to toggle the LED if it is in range. */
	if (uxLED < LED_CNT_MAX) {
		ucLED <<= (u8) uxLED;

		portENTER_CRITICAL();
		{
			if ((ucGPIOState & ucLED) != 0)
				ucGPIOState &= ~ucLED;
			else 
				ucGPIOState |= ucLED;

			XGpio_DiscreteWrite(&xOutputGPIOInstance, 
					uxGPIOOutputChannel, 
					ucGPIOState);
		}
		portEXIT_CRITICAL();
	}
}

