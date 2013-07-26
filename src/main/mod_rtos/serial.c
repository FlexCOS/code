/*
    FreeRTOS V7.1.0 - Copyright (C) 2011 Real Time Engineers Ltd.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.
*/

/*
	BASIC INTERRUPT DRIVEN SERIAL PORT DRIVER FOR a UARTLite peripheral.
*/

#include <const.h>
#include <types.h>
#include <hw.h>

#include <string.h>

#include <modules.h>
#include <array.h>
#include <buffers.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"

/* Library includes. */
#include "xuartlite.h"
#include "xuartlite_l.h"

#include "serial.h"
/*-----------------------------------------------------------*/

/* Functions that are installed as the handler for interrupts that are caused by
Rx and Tx events respectively. */
PRIVATE void serial_rx_handler( void *pvUnused, unsigned portBASE_TYPE uxByteCount );
PRIVATE void serial_tx_handler( void *pvUnused, unsigned portBASE_TYPE uxByteCount );

/* Interface implementation: struct module_io */
PRIVATE const struct array *serial_receive();
PRIVATE void                serial_transmit();

/* Structure that hold the state of the UARTLite peripheral used by this demo.
This is used by the Xilinx peripheral driver API functions. */
PRIVATE XUartLite xUartLiteInstance;

/* The queue used to hold received characters. */
PRIVATE xQueueHandle xRxedChars;

struct ctx {
	u16 expected_bytes;
	u16 received_bytes;
};


/**
 * Initialization hook for module interface.
 *
 * Init serial port interrupts with queue length of 32 byte. 
 */
PUBLIC err_t
serial_io()
{
	static const struct module_io sio = {
		.receive = serial_receive,
		.transmit  = serial_transmit,
	};

	portBASE_TYPE xStatus;

	/* Create the queue used to hold Rx characters.
	 * TODO create config parameter for queue length */
	xRxedChars = xQueueCreate(32, ( unsigned portBASE_TYPE ) sizeof( signed char ));

	/* If the queue was created correctly, then setup the serial port
	hardware. */
	if( xRxedChars != NULL )
	{
		xStatus = XUartLite_Initialize(&xUartLiteInstance, SERIAL_UART_DEVICE_ID);

		if( xStatus == XST_SUCCESS )
		{
			/* Complete initialization of the UART and its associated
			interrupts. */
			XUartLite_ResetFifos( &xUartLiteInstance );
			
			/* Install the handlers that the standard Xilinx library interrupt
			service routine will call when Rx and Tx events occur 
			respectively. */
			XUartLite_SetRecvHandler( &xUartLiteInstance, ( XUartLite_Handler ) serial_rx_handler, NULL );
			XUartLite_SetSendHandler( &xUartLiteInstance, ( XUartLite_Handler ) serial_tx_handler, NULL );
			
			/* Install the standard Xilinx library interrupt handler itself.
			*NOTE* The xPortInstallInterruptHandler() API function must be used 
			for	this purpose. */			
			xStatus = xPortInstallInterruptHandler(
					SERIAL_INTR_ID,
					(XInterruptHandler) XUartLite_InterruptHandler,
					&xUartLiteInstance );
			
			/* Enable the interrupt in the peripheral. */
			XUartLite_EnableIntr( xUartLiteInstance.RegBaseAddress );
			
			/* Enable the interrupt in the interrupt controller.
			*NOTE* The vPortEnableInterrupt() API function must be used for this
			purpose. */
			vPortEnableInterrupt(SERIAL_INTR_ID);
		}

		configASSERT( xStatus == pdPASS );
	}
	
	return module_hal_io_set(&sio);
}

/**
 * Receive one character from Serial Port.
 *
 * @return zero if no character has been received, otherwise one.
 */
PRIVATE inline u8
serial_get(u8 *pcRxedChar)
{
	/* Get the next character from the receive queue.  Return false if no 
	characters are available, or arrive before xBlockTime expires. */
	return xQueueReceive(xRxedChars, pcRxedChar, 0x96);
}

PRIVATE const struct array *
serial_receive()
{
	/* Expect a two bytes header */
	u8  sh[2];
	u8  c;
	u16 bytes_expected;

	while ((!serial_get(&sh[0]) || !serial_get(&sh[1]) ));

	bytes_expected  = sh[0] << 8;
	bytes_expected |= sh[1];

	array_truncate(__capdu);

	// FIXME handle: bytes_expected > __capdu->size

	while (__capdu->length < bytes_expected) {
		if (!serial_get(&c) 
		||  !array_put(__capdu, c))
		{
			// an error occured
		}
	}

	array_truncate(__rapdu);

	return __capdu;
}
/*-----------------------------------------------------------*/

PUBLIC void
serial_transmit()
{
	u8 header[2];

	header[0] = (__rapdu->length & 0xFF00) >> 8;
	header[1] = (__rapdu->length & 0x00FF);

	XUartLite_Send(&xUartLiteInstance, header, sizeof(header));
	XUartLite_Send(&xUartLiteInstance,  __rapdu->v, __rapdu->length);

	return;
}
/*-----------------------------------------------------------*/

PRIVATE void 
serial_rx_handler(void *pvUnused, unsigned portBASE_TYPE uxByteCount)
{
signed char cRxedChar;
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	( void ) pvUnused;
	( void ) uxByteCount;

	/* Place any received characters into the receive queue. */
	while( XUartLite_IsReceiveEmpty( xUartLiteInstance.RegBaseAddress ) == pdFALSE )
	{
		cRxedChar = XUartLite_ReadReg( xUartLiteInstance.RegBaseAddress, XUL_RX_FIFO_OFFSET);
		xQueueSendFromISR( xRxedChars, &cRxedChar, &xHigherPriorityTaskWoken );
	}

	/* If calling xQueueSendFromISR() caused a task to unblock, and the task 
	that unblocked has a priority equal to or greater than the task currently
	in the Running state (the task that was interrupted), then 
	xHigherPriorityTaskWoken will have been set to pdTRUE internally within the
	xQueueSendFromISR() API function.  If xHigherPriorityTaskWoken is equal to
	pdTRUE then a context switch should be requested to ensure that the 
	interrupt returns to the highest priority task that is able	to run. */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
/*-----------------------------------------------------------*/

PRIVATE void
serial_tx_handler(void *pvUnused, unsigned portBASE_TYPE uxByteCount)
{
	( void ) pvUnused;
	( void ) uxByteCount;

	/* Nothing to do here.  The Xilinx library function takes care of the
	transmission. */
	portNOP();
}

