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

#include <xparameters.h>

/* Timer Device for FreeRTOS scheduler 'ticks' */
#define TIMER_DEVICE_ID            XPAR_XPS_TIMER_0_DEVICE_ID
#define TIMER_INTR_ID              XPAR_XPS_INTC_0_XPS_TIMER_0_INTERRUPT_INTR
#define TIMER_BASE_ADDRESS         XPAR_XPS_TIMER_0_BASEADDR
#define TIMER_CLOCK_FREQ           XPAR_XPS_TIMER_0_CLOCK_FREQ_HZ 

#define SERIAL_FLASH               ST_PMOD_FLASH

#define INTC_DEVICE_ID             XPAR_XPS_INTC_0_DEVICE_ID
#define SFLASH_SPI_DEVICE_ID       XPAR_SPI_0_DEVICE_ID

#define AS3953_SPI_DEVICE_ID       XPAR_SPI_0_DEVICE_ID
#define AS3953_INTR_ID             XPAR_XPS_INTC_0_SYSTEM_ANT_IRQ_0_INTR

#define __has_LED                  1
#define LED_CNT_MAX                8
#define LED_GPIO_DEVICE_ID         XPAR_LEDS_8BIT_DEVICE_ID

#define SERIAL_UART_DEVICE_ID      XPAR_UARTLITE_1_DEVICE_ID
#define SERIAL_INTR_ID             XPAR_XPS_INTC_0_RS232_UART_0_INTERRUPT_INTR
