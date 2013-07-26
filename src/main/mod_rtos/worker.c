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

#include <const.h>
#include <types.h>
#include <hw.h>

#include <FreeRTOS.h>
#include <task.h>

#include <led.h>
#include <flexcos.h>

#include <worker.h>

/* Priorities at which the tasks are created. */
#define mainLED_TOGGLE_TASK_PRIORITY            ( tskIDLE_PRIORITY + 2 )
#define mainWORKER_TASK_PRIORITY                ( tskIDLE_PRIORITY + 3 )
#define mainWORKER_STACK_SIZE                   ( 200 )

/* The rate at which data is sent to the queue, specified in milliseconds, and
converted to ticks using the portTICK_RATE_MS constant. */
#define mainQUEUE_SEND_FREQUENCY_MS			( 200 / portTICK_RATE_MS )

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvLedToggleTask(void *pvParameters);
static void prvWorkerTask(void *pvParameters);


PUBLIC err_t
free_rtos_worker_flexcos()
{
	portBASE_TYPE code = xTaskCreate(prvWorkerTask, (signed char *) "Main", 
			mainWORKER_STACK_SIZE, NULL, 
			mainWORKER_TASK_PRIORITY, NULL);

	return code == pdPASS ? E_GOOD : E_INTERN;
}

/**
 *  Start FlexCOS state machine.
 */
PRIVATE void 
prvWorkerTask(void *pvParameters)
{
	flexcos_run();
}

#ifdef __has_LED
PUBLIC err_t
free_rtos_worker_led()
{
	if (led_init())
		return E_HW;
	/* Start the two tasks as described in the comments at the top of this 
	file. */
	portBASE_TYPE code = xTaskCreate(prvLedToggleTask, (signed char *) "LED",
			100, NULL,
			mainLED_TOGGLE_TASK_PRIORITY, NULL);

	return code == pdPASS ? E_GOOD : E_INTERN;
}

/**
 *  Run a binary LED counter.
 */
PRIVATE void
prvLedToggleTask( void *pvParameters )
{
	portTickType xNextWakeTime;
	u8 led_mask = 0;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	loop {
		/* toggle LED directly */
		led_mask %= 15;
		led_set_masked(0x0F, LED_OFF);
		led_set_masked(++led_mask, LED_ON);
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, the constant used converts ticks
		to ms.  While in the Blocked state this task will not consume any CPU
		time. */
		vTaskDelayUntil( &xNextWakeTime, mainQUEUE_SEND_FREQUENCY_MS );
	}
}
#endif
