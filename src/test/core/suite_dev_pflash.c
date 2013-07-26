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

/* Include system libraries */
#include <stdlib.h>

#include <CUnit/Basic.h>
#include <xilflash.h>

#include <const.h>
#include <types.h>
#include <flash_fpga.h>

#include <io/dev.h>
#include <io/dev_pflash.h>

#include <common/test_macros.h>
#include <common/test_utils.h>


static int init_suite(void);
static int clean_suite(void);

static void test_initialize(void);
static void test_overwrite_behavior(void);
static void test_read_bad_offset(void);
static void test_read_with_null_buffer(void);
static void test_read_basic(void);
static void test_read_block(void);
static void test_read_block_across(void);
static void test_self_check(void);
static void test_write_across_block(void);
static void test_write_bad_offset(void);
static void test_write_with_null_buffer(void);
static void test_write_basic(void);
static void test_write_integrity(void);
static void test_write_read_block(void);

static struct mem_dev *FlashDev;

static const union buff_ux i_buff = { .ptr = (void*) _buff_1 };
static const union buff_ux o_buff = { .ptr = (void*) _buff_2 };

/* use XilFlash API for comparison */
static XFlash Flash;
static XFlashGeometry *GeomPtr;

static const struct test_case tc_arr[] = {
	TEST_CASE( test_self_check,        "self check" ),
	TEST_CASE( test_initialize,        "initialize FlashDev"),
	TEST_CASE( test_read_basic,        "basic read" ),
	TEST_CASE( test_write_basic,       "basic write" ),
	TEST_CASE( test_read_block,        "read flash's max block size from start"),
	TEST_CASE( test_read_block_across, "read max flash block size with offset"),
	TEST_CASE( test_write_across_block,"write some bytes across two flash blocks" ),
	TEST_CASE( test_write_read_block,  "write flash's max block size bytes"),
	TEST_CASE( test_overwrite_behavior,"overwrite two bytes" ),
	TEST_CASE( test_write_integrity,   "write keeps neighbor integrity"),
	TEST_CASE( test_read_bad_offset,  "[read] handle bad address" ),
	TEST_CASE( test_write_bad_offset, "[write] handle bad address" ),
	TEST_CASE( test_read_with_null_buffer,   "[read] to NULL buffer" ),
	TEST_CASE( test_write_with_null_buffer,  "[write] from NULL buffer" ),
};



/**
 *
 */
int build_suite__dev_pflash()
{
	INIT_BUILD_SUITE();
	CU_pSuite suite = NULL;

	CREATE_SUITE_OR_DIE("Parallel Flash - MemDev API", suite);
	ADD_TEST_CASES_OR_DIE(suite, tc_arr, (sizeof(tc_arr) / sizeof(*tc_arr)));

	return 0;
}

/* The suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
	int err, i;
	FlashDev = malloc(sizeof(*FlashDev));
	if (FlashDev == NULL)
		return 1;

	err = XFlash_Initialize(
			&Flash,
			FLASH_BASEADDR,
			FLASH_BUS_WIDTH, 0);

	if (err != XST_SUCCESS)
		return 1;

	FlashDev->read = NULL;
	FlashDev->write = NULL;

	GeomPtr = &Flash.Geometry;

	for (i = 0; i < BUFF32_LENGTH; i++)
		o_buff.u32[i] = OxDEADBEAF.get;
	return 0;
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
	free( FlashDev );
	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Test Cases                                                                */
/* -------------------------------------------------------------------------- */
inline static void sub_test_is_initialized() {
	CU_ASSERT_PTR_NOT_NULL_FATAL ( FlashDev );
	CU_ASSERT_PTR_NOT_NULL_FATAL ( FlashDev->read );
	CU_ASSERT_PTR_NOT_NULL_FATAL ( FlashDev->write );
}

static void test_self_check(void)
{
	CU_ASSERT_PTR_NOT_NULL ( GeomPtr );
	CU_ASSERT_PTR_NOT_NULL ( i_buff.ptr );
	CU_ASSERT_PTR_NOT_NULL ( o_buff.ptr );
	CU_ASSERT_PTR_NOT_EQUAL ( i_buff.ptr, o_buff.ptr );
	CU_ASSERT_EQUAL ( o_buff.u32[0], OxDEADBEAF.get );
}

static void test_initialize(void)
{
	int err;

	CU_ASSERT_PTR_NOT_NULL ( FlashDev );

	err = dev_pflash_init(FlashDev);
	CU_ASSERT_EQUAL ( err, E_GOOD );

	sub_test_is_initialized();
}

static void test_read_basic(void)
{
	int err;
	u32 x, y;

	sub_test_is_initialized();

	err = FlashDev->read(0, 4, DEST(&x));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = XFlash_Read(&Flash, 0, 4, DEST(&y));
	CU_ASSERT_EQUAL ( err, XST_SUCCESS );

	CU_ASSERT_EQUAL ( x, y );
}

static void test_read_block(void)
{
	int err;

	CU_ASSERT_TRUE_FATAL ( BUFF_BYTES >= FLASH_BLOCK_SIZE_MAX );

	err = FlashDev->read(0, FLASH_BLOCK_SIZE_MAX, DEST(i_buff.u8));
	CU_ASSERT_EQUAL ( err, E_GOOD );
}

static void test_read_block_across(void)
{
	int err;

	CU_ASSERT_TRUE_FATAL ( BUFF_BYTES >= FLASH_BLOCK_SIZE_MAX );

	err = FlashDev->read(16, FLASH_BLOCK_SIZE_MAX, DEST(i_buff.u8));
	CU_ASSERT_EQUAL ( err, E_GOOD );
}

static void test_write_basic(void)
{
	int err;
	u32 _deadbeaf;
	const u32 offset = 128;
	const u32 nbytes = sizeof(OxDEADBEAF);

	err = FlashDev->write(offset, nbytes, SRC(&OxDEADBEAF));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = FlashDev->read(offset, nbytes, DEST(&_deadbeaf));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL ( _deadbeaf, OxDEADBEAF.get);

	err = XFlash_Read(&Flash, offset, nbytes, DEST(&_deadbeaf));
	CU_ASSERT_EQUAL ( _deadbeaf, OxDEADBEAF.get );
}

static void test_write_across_block(void)
{
	int err;
	u32 _deadbeaf;
	const u32 offset = GeomPtr->EraseRegion[0].Size - 2;
	const u32 nbytes = sizeof(OxDEADBEAF.get);

	err = FlashDev->write(offset, nbytes, SRC(&OxDEADBEAF));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = FlashDev->read(offset, nbytes, DEST(&_deadbeaf));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL ( _deadbeaf, OxDEADBEAF.get );

	err = XFlash_Read(&Flash, offset, nbytes, DEST(&_deadbeaf));
	CU_ASSERT_EQUAL ( _deadbeaf, OxDEADBEAF.get );
}

static void test_write_read_block(void)
{
	int err, i;

	err = FlashDev->write(16, BUFF_BYTES, SRC(o_buff.ptr));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = FlashDev->read(16, BUFF_BYTES, DEST(i_buff.ptr));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	for (i = 0; i < BUFF32_LENGTH; i++)
		if ( o_buff.u32[i] != i_buff.u32[i] )
			CU_FAIL_FATAL ("i_buff != o_buff");
}

static void test_read_with_null_buffer(void)
{
	int err;

	err = FlashDev->read(0, 4, NULL);
	CU_ASSERT_TRUE ( err );
}

static void test_write_with_null_buffer(void)
{
	int err;

	err = FlashDev->write(0, 4, NULL);
	CU_ASSERT_TRUE ( err );
}

static void test_read_bad_offset(void)
{
	int err;
	u16 foo = 0;

	/* write last two bytes */
	err = FlashDev->write(FLASH_SIZE-2, 2, SRC(&OxDEAD));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = FlashDev->read(FLASH_SIZE-2, 4, DEST(&foo));
	/* expect an error at all */
	CU_ASSERT_TRUE ( err );
	CU_ASSERT_TRUE ( eclass(err), E_BAD_PARAM );
	CU_ASSERT_TRUE ( ecode(err),  E_ADDRESS );
	/* no partial read */
	CU_ASSERT_EQUAL ( foo, 0 );

	err = FlashDev->read(FLASH_SIZE, 4, DEST(&foo));
	CU_ASSERT_TRUE ( err );
	CU_ASSERT_TRUE ( eclass(err), E_BAD_PARAM );
	CU_ASSERT_TRUE ( ecode(err),  E_ADDRESS );
}

static void test_write_bad_offset(void)
{
	int err;
	u16 _beaf = 0;

	/* write last two bytes */
	err = FlashDev->write(FLASH_SIZE-2, 2, SRC(&OxBEAF));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	/* proceeding this unit test is useless if '0xbeaf'
	 * was not successful written */
	err = FlashDev->read(FLASH_SIZE-2,2, DEST(&_beaf));
	CU_ASSERT_EQUAL_FATAL ( _beaf, OxBEAF.get );

	/* try to overwrite last written '0xbeaf' with '0xdead'  */
	err = FlashDev->write(FLASH_SIZE-2, 4, SRC(&OxDEADBEAF));
	/* expect an error at all */
	CU_ASSERT_TRUE ( err );
	CU_ASSERT_TRUE ( eclass(err), E_BAD_PARAM );
	CU_ASSERT_TRUE ( ecode(err),  E_ADDRESS );

	/* no partial write */
	err = FlashDev->read(FLASH_SIZE-2,2, DEST(&_beaf));
	CU_ASSERT_EQUAL ( _beaf, OxBEAF.get );

	err = FlashDev->write(FLASH_SIZE, 4, DEST(&OxDEADBEAF));
	CU_ASSERT_TRUE ( err );
	CU_ASSERT_TRUE ( eclass(err), E_BAD_PARAM );
	CU_ASSERT_TRUE ( ecode(err),  E_ADDRESS );
}

static void test_overwrite_behavior(void)
{
	int err;
	u16 val16;

	err = FlashDev->write(12, 2, SRC(&OxDEAD));
	CU_ASSERT_EQUAL ( err, E_GOOD );
	err = FlashDev->read(12, 2, DEST(&val16));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL ( val16, OxDEAD.get );

	/* overwrite 0xdead with 0xbeaf */
	err = FlashDev->write(12, 2, SRC(&OxBEAF));
	CU_ASSERT_EQUAL ( err, E_GOOD );
	err = FlashDev->read(12, 2, DEST(&val16));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL ( val16, OxBEAF.get );
}

static void test_write_integrity(void)
{
	int err;
	union blob_u32 left, right;
	const u32 offset = 256;

	left.set = OxDEADBEAF.get;
	right.set = OxDEADBEAF.get;

	err = FlashDev->write(offset-2, 4, SRC(&left.get));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = FlashDev->write(offset+BUFF_BYTES-2, 4, SRC(&right.get));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL ( o_buff.u16[0], OxDEAD.get );
	CU_ASSERT_EQUAL ( o_buff.u16[BUFF16_LENGTH-1], OxBEAF.get );

	/* this should overwrite the last two bytes of written left identifier
	 * and first two written bytes of right identifier. */
	err = FlashDev->write(offset, BUFF_BYTES, SRC(o_buff.ptr));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = FlashDev->read(offset-2, 4, DEST(&left.set));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = FlashDev->read(offset+BUFF_BYTES-2, 4, DEST(&right.set));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL ( left.as_u16._1, OxDEAD.get );
	CU_ASSERT_EQUAL ( left.as_u16._2, OxDEAD.get );
	CU_ASSERT_EQUAL ( right.as_u16._1, OxBEAF.get );
	CU_ASSERT_EQUAL ( right.as_u16._2, OxBEAF.get );
}
