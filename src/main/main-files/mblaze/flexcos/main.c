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

/* Config and constant includes. */
#include <const.h>
#include <types.h>

#include <modules.h>

/* Kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>

#include <microblaze.h>

/* Application includes. */
#include "serial.h"
#include "as3953_io.h"
#include "flexcos.h"

#include <worker.h>

PRIVATE err_t
free_rtos_worker_as3953()
{
	/**
	 * Initializes SPI Controller and sets hal IO function pointers.
	 */
	if (mod_iso14443_io() != E_GOOD) return E_FAILED;

	portBASE_TYPE code = xTaskCreate(as3953_io_state_machine,
			(signed char *) "AS3953", 250, NULL, 3 + 1, NULL);

	return code == pdPASS ? E_GOOD : E_INTERN;
}

PRIVATE err_t
free_rtos_worker_as3953_3()
{
	/**
	 * Initializes SPI Controller and sets hal IO function pointers.
	 */
	if (mod_iso14443_io_3() != E_GOOD) return E_FAILED;

	portBASE_TYPE code = xTaskCreate(as3953_io_state_machine_3,
			(signed char *) "AS3953", 250, NULL, 3 + 1, NULL);

	return code == pdPASS ? E_GOOD : E_INTERN;
}

MODULES(microblaze_sanitize_cache,
        free_rtos_worker_as3953,
        free_rtos_worker_led,
        free_rtos_worker_flexcos
        );

int main( void )
{
	taskDISABLE_INTERRUPTS();
	/* Configure the interrupt controller, LED outputs and button inputs. */
	if (modules_init() != E_GOOD) {
		halt();
	}

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details. */
	halt();
}
/*-----------------------------------------------------------*/


