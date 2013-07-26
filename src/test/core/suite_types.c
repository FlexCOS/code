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
#include <const.h>
#include <types.h>

#include <common/test_macros.h>
#include <common/test_utils.h>

static int init_suite(void);
static int clean_suite(void);

static void test_expected_size(void);
static void test_dead_and_beaf(void);
static void test_MIN(void);
static void test_MAX(void);


static const struct test_case tc_arr[] = {
	TEST_CASE( test_expected_size, "expected size of data types" ),
	TEST_CASE( test_MIN, "MIN() macro" ),
	TEST_CASE( test_MAX, "MAX() macro" ),
	TEST_CASE( test_dead_and_beaf, "some dead beaf" )
};



/**
 *
 *
 */
int build_suite__types()
{
	INIT_BUILD_SUITE();
	CU_pSuite pSuite = NULL;

	CREATE_SUITE_OR_DIE("Types and tools", pSuite);
	ADD_TEST_CASES_OR_DIE(pSuite, tc_arr);

	return 0;
}

/* The suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
	/* no initialization required */
	return 0;
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
	return 0;
}

/*
 * Simple test ...
 */
static void test_expected_size(void)
{
	CU_ASSERT_EQUAL( 1, sizeof(u8)   );
	CU_ASSERT_EQUAL( 2, sizeof(u16)  );
	CU_ASSERT_EQUAL( 4, sizeof(u32)  );
	CU_ASSERT_EQUAL( 2, sizeof(fid_t));
}

static void test_MIN(void)
{
	CU_ASSERT_EQUAL( MIN(1, 2), 1 );
	CU_ASSERT_EQUAL( MIN(2, 1), 1 );
	CU_ASSERT_EQUAL( MIN(3, 3), 3 );
}

static void test_MAX(void)
{
	CU_ASSERT_EQUAL( MAX(1, 2), 2 );
	CU_ASSERT_EQUAL( MAX(2, 1), 2 );
	CU_ASSERT_EQUAL( MAX(3, 3), 3 );
}

static void test_dead_and_beaf(void)
{
	CU_ASSERT_EQUAL ( sizeof(0xDEADBEAF), 4 );
	CU_ASSERT_EQUAL ( sizeof(0xDEAD),     2 );
	CU_ASSERT_EQUAL ( sizeof(0xBEAF),     2 );

	CU_ASSERT_EQUAL ( OxDEADBEAF.head16, OxDEAD.get );
	CU_ASSERT_EQUAL ( OxDEADBEAF.head8, OxDEAD.head8 );
	CU_ASSERT_EQUAL ( OxDEADBEAF.as_u16._2, OxBEAF.get );
	CU_ASSERT_EQUAL ( OxDEADBEAF.as_u8._3, OxBEAF.head8 );
	CU_ASSERT_EQUAL ( OxDEADBEAF.as_u8._4, OxBEAF.as_u8._2 );
}

