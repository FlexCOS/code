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

#include <flxlib.h>
#include <i7816.h>
#include <flxio.h>
#include <CUnit/Basic.h>

#include <common/test_macros.h>
#include <common/test_utils.h>

#include "stub_fs.h"

static int init_suite();
static int clean_suite();

/*===========================================================================*
   Module contstants
 *===========================================================================*/
#define TEST_FID 0x2211
#define TEST_SIZE 4

PRIVATE fid_t _test_path[] = { TEST_FID, EOP};
PRIVATE path_t test_path = _test_path;
PRIVATE const size_t test_file_size = TEST_SIZE;
PRIVATE struct i7_fcp new_file = {
	.fid  = TEST_FID,
	.fdb  = 0x01,
	.size = TEST_SIZE,
};

/*===========================================================================*
   Module variables
 *===========================================================================*/
PRIVATE FILE *test_file;


/*===========================================================================*
   Prototype definitions
 *===========================================================================*/
PRIVATE void test_create_file(void);
PRIVATE void test_open_file(void);
PRIVATE void test_close_file(void);
PRIVATE void test_read_file(void);
PRIVATE void test_write_file(void);

/*===========================================================================*
   Test case definitions
 *===========================================================================*/
static const struct test_case tc_arr[] = {
	TEST_CASE ( test_create_file, "create a file" ),
	TEST_CASE ( test_close_file, "close a file"),
	TEST_CASE ( test_open_file, "open a file"),
	TEST_CASE ( test_write_file, "write a file"),
	TEST_CASE ( test_read_file, "read a file"),
};



/*===========================================================================*
   Public suite initialisation functions
 *===========================================================================*/
int build_suite__flxio()
{
	INIT_BUILD_SUITE();
	CU_pSuite pSuite = NULL;

	CREATE_SUITE_OR_DIE("flxio API", pSuite);
	ADD_TEST_CASES_OR_DIE(pSuite, tc_arr);

	return 0;
}

/* The suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
static int
init_suite(void)
{
	test_file = NULL;
	return stub_fs_init();
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
static int
clean_suite(void)
{
	return stub_fs_free();
}

/*===========================================================================*
   Test case implementations
 *===========================================================================*/
PRIVATE void
test_create_file(void)
{
	test_file = f_create(&new_file);
	CU_ASSERT_PTR_NOT_NULL (test_file);
}

PRIVATE void
test_close_file(void)
{
	CU_ASSERT_PTR_NOT_NULL_FATAL (test_file);
	CU_ASSERT_EQUAL (f_close(test_file), E_GOOD);
}

PRIVATE void
test_open_file(void)
{
	FILE *fh;

	fh = f_open(test_path);
	CU_ASSERT_PTR_NOT_NULL_FATAL (fh);

	CU_ASSERT_EQUAL (f_close(fh), E_GOOD);
}

PRIVATE void
test_write_file(void)
{
	FILE   *fh;
	size_t bytes;

	fh = f_open(test_path);
	CU_ASSERT_PTR_NOT_NULL_FATAL (fh);

	/* REMINDER: test_file_size is set to 4 */

	bytes = f_write(SRC(&OxBEAF), 2, 1, fh);
	CU_ASSERT_EQUAL (bytes, 2);
	bytes = f_write(SRC(&OxDEADBEAF), 4, 1, fh);
	CU_ASSERT_EQUAL (bytes, 2);

	CU_ASSERT_EQUAL (f_close(fh), E_GOOD);
}

PRIVATE void
test_read_file(void)
{
	union blob_u16 _beaf = { .set = 0 };
	union blob_u32 _deadbeaf = { .set = 0 };

	FILE   *fh;
	size_t bytes;

	fh = f_open(test_path);
	CU_ASSERT_PTR_NOT_NULL_FATAL (fh);

	/* REMINDER: test_file_size is set to 4 */

	bytes = f_read(DEST(&_beaf), 2, 1, fh);
	CU_ASSERT_EQUAL (bytes, 2);
	CU_ASSERT_EQUAL (_beaf.get, OxBEAF.get);
	bytes = f_read(DEST(&_deadbeaf), 4, 1, fh);
	CU_ASSERT_EQUAL (bytes, 2);
	CU_ASSERT_EQUAL (_deadbeaf.as_u16._1, OxDEAD.get);
	CU_ASSERT_EQUAL (_deadbeaf.as_u16._2, 0);

	CU_ASSERT_EQUAL (f_close(fh), E_GOOD);
}
