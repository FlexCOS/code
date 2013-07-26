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

#include <flash_layout.h>

#include <common/test_macros.h>
#include <common/test_utils.h>

PRIVATE struct flash_layout __test_layout = {
	.total_size = 0x1000000,
	.clusters   = 1,
	.cluster = {
		FLASH_CLUSTER(0, 256, 256, 256),
		FLASH_CLUSTER_UNUSED_SLOT,
		FLASH_CLUSTER_UNUSED_SLOT,
		FLASH_CLUSTER_UNUSED_SLOT
	}
};

static int init_suite(void);
static int clean_suite(void);

static void test_layout_locate(void);
static void test_layout_contains(void);
static void test_loc_relocate(void);
static void test_loc_is_sound(void);
static void test_loc_sector_start(void);
static void test_loc_sector_end(void);


static const struct test_case tc_arr[] = {
	TEST_CASE( test_layout_contains, "is an address within layout definition" ),
	TEST_CASE( test_layout_locate, "locate an address" ),
	TEST_CASE( test_loc_relocate, "update absolute address of a flash_loc"),
	TEST_CASE( test_loc_is_sound, "soundness of a location" ),
	TEST_CASE( test_loc_sector_start, "get start of sector" ),
	TEST_CASE( test_loc_sector_end, "get end of sector" ),
//	TEST_CASE( test_loc_sector_next, "get start of next sector" ),
//	TEST_CASE( test_loc_sector_prev, "get start of previous sector" )
};

/**
 *
 *
 */
int build_suite__flash_layout()
{
	INIT_BUILD_SUITE();
	CU_pSuite pSuite = NULL;

	CREATE_SUITE_OR_DIE("Flash-Layout Abstraction", pSuite);
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

/**
 *
 */
PRIVATE void
test_layout_contains(void)
{
	const struct flash_layout *l = &__test_layout;

	/* has the layout unexpectedly been changed */
	CU_ASSERT_EQUAL( l->total_size, 0xFFFFFF + 1 );
	
	CU_ASSERT_TRUE( flash_layout__contains(l, 0x00) );
	CU_ASSERT_TRUE( flash_layout__contains(l, 0xff) );
	CU_ASSERT_TRUE( flash_layout__contains(l, 0xff00) );
	CU_ASSERT_TRUE( flash_layout__contains(l, 0xff) );
	CU_ASSERT_TRUE( flash_layout__contains(l, 0xFFFFFF) );

	CU_ASSERT_FALSE( flash_layout__contains(l, l->total_size) );
}

PRIVATE void
test_layout_locate(void)
{
	const struct flash_layout *l = &__test_layout;
	struct flash_loc loc = {0xFF};

	CU_ASSERT_EQUAL( flash_layout__locate(l, 0, &loc), E_GOOD );
	CU_ASSERT_EQUAL( loc.abs, 0 );
	CU_ASSERT_EQUAL( loc.c, 0 );
	CU_ASSERT_EQUAL( loc.s, 0 );
	CU_ASSERT_EQUAL( loc.p, 0 );
	CU_ASSERT_EQUAL( loc.o, 0 );
	CU_ASSERT_PTR_EQUAL(loc.ly, l);

	CU_ASSERT_EQUAL( flash_layout__locate(l, 4, &loc), E_GOOD );
	CU_ASSERT_EQUAL( loc.c, 0 );
	CU_ASSERT_EQUAL( loc.s, 0 );
	CU_ASSERT_EQUAL( loc.p, 0 );
	CU_ASSERT_EQUAL( loc.o, 4 );
	CU_ASSERT_PTR_EQUAL(loc.ly, l);

	CU_ASSERT_EQUAL( flash_layout__locate(l, 0x020405, &loc), E_GOOD);
	CU_ASSERT_EQUAL( loc.abs, 0x020405 );
	CU_ASSERT_EQUAL( loc.c, 0 );
	CU_ASSERT_EQUAL( loc.s, 2 );
	CU_ASSERT_EQUAL( loc.p, 4 );
	CU_ASSERT_EQUAL( loc.o, 5 );
	CU_ASSERT_PTR_EQUAL(loc.ly, l);

	CU_ASSERT_NOT_EQUAL( flash_layout__locate(l, 0x01000000, &loc), E_GOOD );
}

PRIVATE void
test_loc_relocate(void)
{
	const struct flash_layout *l = &__test_layout;
	struct flash_loc loc= {0};
	
	loc.ly = l;
	loc.o  = 0x10;

	CU_ASSERT_EQUAL( flash_loc__relocate(&loc), E_GOOD );
	CU_ASSERT_EQUAL( loc.abs, 0x000010 );

	loc.s = 0x1F;
	loc.p = 0x39;
	loc.o = 0x22;
	CU_ASSERT_EQUAL( flash_loc__relocate(&loc), E_GOOD );
	CU_ASSERT_EQUAL( loc.abs, 0x1F3922 );
}

PRIVATE void
test_loc_is_sound(void)
{
	CU_FAIL("Not implemented");
}

PRIVATE void
test_loc_sector_start(void)
{
	const struct flash_layout *l = &__test_layout;
	struct flash_loc loc= {0};
	struct flash_loc s;
	
	loc.ly = l;
	loc.abs = 0x302010;
	loc.s   = 0x30;
	loc.p   = 0x20;
	loc.o   = 0x10;
	CU_ASSERT_EQUAL( flash_loc__sector_start(&loc, &s), E_GOOD );
	CU_ASSERT_EQUAL( loc.s, s.s );
	CU_ASSERT_EQUAL( s.p, 0 );
	CU_ASSERT_EQUAL( s.o, 0 );
	CU_ASSERT_EQUAL( s.abs, 0x300000 );
	CU_ASSERT_PTR_EQUAL( loc.ly, s.ly );
	
	loc.abs = 0xF00000;
	loc.s   = 0xF0;
	loc.p   = 0x00;
	loc.o   = 0x00;
	CU_ASSERT_EQUAL( flash_loc__sector_start(&loc, &s), E_GOOD );
	CU_ASSERT_EQUAL( loc.abs, s.abs );
	CU_ASSERT_EQUAL( s.s, 0xF0 );
	CU_ASSERT_EQUAL( s.p, 0x00 );
	CU_ASSERT_EQUAL( s.o, 0x00 );
}

PRIVATE void
test_loc_sector_end(void)
{
	const struct flash_layout *l = &__test_layout;
	struct flash_loc loc= {0};
	struct flash_loc s;
	
	loc.ly = l;
	loc.abs = 0x302010;
	loc.s   = 0x30;
	loc.p   = 0x20;
	loc.o   = 0x10;
	CU_ASSERT_EQUAL( flash_loc__sector_end(&loc, &s), E_GOOD );
	CU_ASSERT_EQUAL( loc.s, s.s );
	CU_ASSERT_EQUAL( s.p, 0xFF );
	CU_ASSERT_EQUAL( s.o, 0xFF );
	CU_ASSERT_EQUAL( s.abs, 0x30FFFF );
	CU_ASSERT_PTR_EQUAL( loc.ly, s.ly );
	
	loc.abs = 0xF0FFFF;
	loc.s   = 0xF0;
	loc.p   = 0xFF;
	loc.o   = 0xFF;
	CU_ASSERT_EQUAL( flash_loc__sector_end(&loc, &s), E_GOOD );
	CU_ASSERT_EQUAL( loc.abs, s.abs );
	CU_ASSERT_EQUAL( s.s, 0xF0 );
	CU_ASSERT_EQUAL( s.p, 0xFF );
	CU_ASSERT_EQUAL( s.o, 0xFF );

	loc.s = 0x100;
	CU_ASSERT_NOT_EQUAL( flash_loc__sector_end(&loc, &s), E_GOOD );
}
