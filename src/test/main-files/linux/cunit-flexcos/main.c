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

#include <pthread.h>

#include <const.h>
#include <types.h>
#include <modules.h>

#include <flexcos.h>

#include <common/pipe_io.h>
#include "core/suites.h"

PRIVATE err_t  thread_flexcos();
PRIVATE void * flexcos(void *);


MODULES(pipe_io,
        thread_flexcos);

int
main(void)
{
	/* setup basic hardware */
	if (modules_init() != E_GOOD) halt();

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
	pthread_exit(NULL);
}

PRIVATE void *
flexcos(void *ptr)
{
	flexcos_run();

	pthread_exit(NULL);
}

PRIVATE err_t
thread_flexcos()
{
	pthread_t thread;

	if (pthread_create(&thread, NULL, flexcos, NULL))
		return E_SYSTEM;
	
	return E_GOOD;
}
