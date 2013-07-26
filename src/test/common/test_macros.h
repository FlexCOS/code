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
#ifndef _TEST_MACROS_H
#define _TEST_MACROS_H

/** A test case is just a function without any parameter and return value */
typedef void(*tcase_t)(void);

/** CUnit registers test functions with a descriptive text */
struct test_case {
	tcase_t fun;
	char*   text;
};

/** ADD_TEST_CASE_OR_DIE macro loops over an array of test cases.
 *  Call this macro in order to declare required identifier at the
 *  beginning of any 'build_suite__*()' function. */
#define INIT_BUILD_SUITE() int _i = 0

/* create an initialized test_case structure */
#define TEST_CASE(f, t) { .fun = f, .text = t }

/** Call 'CU_add_suite' or die */
#define CREATE_SUITE_OR_DIE( suite_name, pSuite ) do { \
	pSuite = CU_add_suite( suite_name , init_suite, clean_suite ); \
	if (NULL == pSuite) { \
		CU_cleanup_registry(); \
		return E_MEMORY; \
	}} while(0)


#define ADD_TEST_CASES_OR_DIE(suite, tc_arr) do {\
	for (_i = 0; _i < LENGTH(tc_arr); _i++) { \
		if (NULL == CU_add_test(suite, tc_arr[_i].text, tc_arr[_i].fun)) { \
			CU_cleanup_registry(); \
			return E_MEMORY; \
		} \
	}} while(0)

#endif /* ----- end of macro protection ----- */
