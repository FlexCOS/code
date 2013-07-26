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

#include <CUnit/Basic.h>
#include <FreeRTOS.h>
#include <task.h>

#include <const.h>
#include <types.h>
#include <modules.h>

#include <microblaze.h>

#include <led.h>
#include <loop_io.h>
#include <worker.h>

#include "core/suites.h"

PRIVATE err_t free_rtos_worker_run_tests();


MODULES(microblaze_sanitize_cache,
	led_init,
	loop_io,
	free_rtos_worker_flexcos,
	free_rtos_worker_led,
	free_rtos_worker_run_tests);

int
main(void)
{
	/* setup basic hardware */
	if (modules_init() != E_GOOD) halt();

	vTaskStartScheduler();

	/* This should not be reached */
	halt();
}

PRIVATE void
run_tests(void* foo)
{
	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		goto halt;

	/* create suites, order is relevant */
	if (build_suite__types() ||
	    build_suite__flexcos())
	{
		goto halt;
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();

halt:
	halt();
}

PRIVATE err_t
free_rtos_worker_run_tests()
{
	xTaskCreate(run_tests,
	            (signed char *) "CUnit", 
	            1000, 
		    NULL, 
		    tskIDLE_PRIORITY + 3, 
		    NULL);

	return E_GOOD;
}


