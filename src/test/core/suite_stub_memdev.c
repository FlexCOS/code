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

#include <const.h>
#include <types.h>

#include <io/dev.h>

#include <common/test_macros.h>
#include <common/test_utils.h>

#include "stub_memdev.h"


static int init_suite(void);
static int clean_suite(void);

static void test_initialize(void);
static void test_overwrite_behavior(void);
static void test_read_bad_offset(void);
static void test_read_basic(void);
static void test_read_buff_bytes(void);
static void test_read_with_null_buffer(void);
static void test_self_check(void);
static void test_write_bad_offset(void);
static void test_write_basic(void);
static void test_write_integrity(void);
static void test_write_read_buff_bytes(void);
static void test_write_with_null_buffer(void);
static void test_release(void);

static struct mem_dev *StubDev;

static const union buff_ux i_buff = { .ptr = (void*) _buff_1 };
static const union buff_ux o_buff = { .ptr = (void*) _buff_2 };

static const struct test_case tc_arr[] = {
	TEST_CASE( test_self_check,        "self check" ),
	TEST_CASE( test_initialize,        "initialize StubDev"),
	TEST_CASE( test_read_basic,        "basic read" ),
	TEST_CASE( test_write_basic,       "basic write" ),
	TEST_CASE( test_read_buff_bytes,   "read a block of 'BUFF_BYTES'"),
	TEST_CASE( test_write_read_buff_bytes, "write BUFF_BYTES with read check" ),
	TEST_CASE( test_overwrite_behavior,"overwrite two bytes" ),
	TEST_CASE( test_write_integrity,   "write keeps neighbor integrity"),
	TEST_CASE( test_read_bad_offset,       "[read] handle bad address" ),
	TEST_CASE( test_write_bad_offset,      "[write] handle bad address" ),
	TEST_CASE( test_read_with_null_buffer, "[read] to NULL buffer" ),
	TEST_CASE( test_write_with_null_buffer,"[write] from NULL buffer" ),
	TEST_CASE( test_release,               "release StubDev" )
};



/**
 *
 */
int build_suite__stub_memdev()
{
	INIT_BUILD_SUITE();
	CU_pSuite suite = NULL;

	CREATE_SUITE_OR_DIE("Stub Memory Device - MemDev API", suite);
	ADD_TEST_CASES_OR_DIE(suite, tc_arr);

	return 0;
}

/* The suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
	static struct mem_dev dev;
	int i;

	StubDev = &dev;

	StubDev->size = 0;
	StubDev->read = NULL;
	StubDev->write = NULL;

	for (i = 0; i < BUFF32_LENGTH; i++)
		o_buff.u32[i] = OxDEADBEAF.get;

	return 0;
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Test Cases                                                                */
/* -------------------------------------------------------------------------- */
inline static void sub_test_is_initialized() {
	CU_ASSERT_PTR_NOT_NULL_FATAL ( StubDev );
	CU_ASSERT_NOT_EQUAL_FATAL ( StubDev->size, 0 );
	CU_ASSERT_PTR_NOT_NULL_FATAL ( StubDev->read );
	CU_ASSERT_PTR_NOT_NULL_FATAL ( StubDev->write );
}

static void test_self_check(void)
{
	CU_ASSERT_PTR_NOT_NULL ( i_buff.ptr );
	CU_ASSERT_PTR_NOT_NULL ( o_buff.ptr );
	CU_ASSERT_PTR_NOT_EQUAL ( i_buff.ptr, o_buff.ptr );
	CU_ASSERT_EQUAL ( o_buff.u32[0], OxDEADBEAF.get);
	CU_ASSERT_EQUAL ( o_buff.u32[BUFF32_LENGTH-1], OxDEADBEAF.get );
}

static void test_initialize(void)
{
	int err;

	CU_ASSERT_PTR_NOT_NULL ( StubDev );

	err = stub_memdev_init(StubDev);
	CU_ASSERT_EQUAL ( err, E_GOOD );
	CU_ASSERT_TRUE ( BUFF_BYTES <= StubDev->size );

	sub_test_is_initialized();
}

static void test_read_basic(void)
{
	int err;
	u32 x = 0xFFFFFFFF;

	sub_test_is_initialized();

	err = StubDev->read(0, 4, DEST(&x));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	/* Stub device is expected to be set to zero */
	CU_ASSERT_EQUAL ( x, 0x00 );
}

static void test_read_buff_bytes(void)
{
	int err;

	CU_ASSERT_TRUE_FATAL ( BUFF_BYTES <= StubDev->size );

	err = StubDev->read(0, BUFF_BYTES, DEST(i_buff.u8));
	CU_ASSERT_EQUAL ( err, E_GOOD );
}

static void test_write_basic(void)
{
	int err;
	u32 _deadbeaf;
	const u32 offset = 128;
	const u32 nbytes = sizeof(OxDEADBEAF.get);

	err = StubDev->write(offset, nbytes, SRC(&OxDEADBEAF.get));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = StubDev->read(offset, nbytes, DEST(&_deadbeaf));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL ( _deadbeaf, OxDEADBEAF.get );
}

static void test_write_read_buff_bytes(void)
{
	int err, i;

	err = StubDev->write(16, BUFF_BYTES, SRC(o_buff.ptr));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = StubDev->read(16, BUFF_BYTES, DEST(i_buff.ptr));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	for (i = 0; i < BUFF32_LENGTH; i++)
		if ( o_buff.u32[i] != i_buff.u32[i] )
			CU_FAIL_FATAL ("i_buff != o_buff");
}

static void test_read_with_null_buffer(void)
{
	err_t err;

	err = StubDev->read(0, 4, NULL);
	CU_ASSERT_TRUE  ( err );
	CU_ASSERT_EQUAL ( eclass(err), E_BAD_PARAM );
	CU_ASSERT_EQUAL ( ecode(err),  E_NULL_PTR );
}

static void test_write_with_null_buffer(void)
{
	int err;

	err = StubDev->write(0, 4, NULL);
	CU_ASSERT_NOT_EQUAL ( err, E_GOOD );
	CU_ASSERT_TRUE ( err );
	CU_ASSERT_EQUAL ( eclass(err), E_BAD_PARAM );
	CU_ASSERT_EQUAL ( ecode(err), E_NULL_PTR );
}

static void test_read_bad_offset(void)
{
	int err;
	u16 foo = 0;

	/* write last two bytes */
	err = StubDev->write(StubDev->size-2, 2, SRC(&OxDEAD));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = StubDev->read(StubDev->size-2, 4, DEST(&foo));
	/* expect an error at all */
	CU_ASSERT_TRUE  ( err );
	CU_ASSERT_EQUAL ( eclass(err), E_BAD_PARAM );
	CU_ASSERT_EQUAL ( ecode(err),  E_ADDRESS );
	/* no partial read */
	CU_ASSERT_EQUAL ( foo, 0 );

	err = StubDev->read(StubDev->size, 4, DEST(&foo));
	CU_ASSERT_TRUE  ( err );
	CU_ASSERT_EQUAL ( eclass(err), E_BAD_PARAM );
	CU_ASSERT_EQUAL ( ecode(err),  E_ADDRESS );
}

static void test_write_bad_offset(void)
{
	int err;
	u16 _beaf = 0;

	/* write last two bytes */
	err = StubDev->write(StubDev->size-2, 2, SRC(&OxBEAF));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	/* proceeding this unit test is useless if '0xbeaf'
	 * was not successful written */
	err = StubDev->read(StubDev->size-2,2, DEST(&_beaf));
	CU_ASSERT_EQUAL_FATAL ( _beaf, OxBEAF.get );

	/* try to overwrite last written '0xbeaf' with '0xdead'  */
	err = StubDev->write(StubDev->size-2, 4, SRC(&OxDEADBEAF));
	CU_ASSERT_TRUE  ( err );
	CU_ASSERT_EQUAL ( eclass(err), E_BAD_PARAM );
	CU_ASSERT_EQUAL ( ecode(err),  E_ADDRESS );

	/* no partial write */
	err = StubDev->read(StubDev->size-2,2, DEST(&_beaf));
	CU_ASSERT_EQUAL ( _beaf, OxBEAF.get );

	err = StubDev->write(StubDev->size, 4, DEST(&OxDEADBEAF));
	CU_ASSERT_TRUE  ( err );
	CU_ASSERT_EQUAL ( eclass(err), E_BAD_PARAM );
	CU_ASSERT_EQUAL ( ecode(err),  E_ADDRESS );
}

static void test_overwrite_behavior(void)
{
	int err;
	u16 val16;

	err = StubDev->write(12, 2, SRC(&OxDEAD.get));
	CU_ASSERT_EQUAL ( err, E_GOOD );
	err = StubDev->read(12, 2, DEST(&val16));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL ( val16, OxDEAD.get );

	/* overwrite 0xdead with 0xbeaf */
	err = StubDev->write(12, 2, SRC(&OxBEAF.get));
	CU_ASSERT_EQUAL ( err, E_GOOD );
	err = StubDev->read(12, 2, DEST(&val16));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL ( val16, OxBEAF.get );
}

static void test_write_integrity(void)
{
	int err;
	union blob_u32 left, right;
	const u32 offset = 256;

	CU_ASSERT_TRUE_FATAL ( (offset+BUFF_BYTES+4) <= StubDev->size );

	left.set = right.set = OxDEADBEAF.get;

	err = StubDev->write(offset-2, 4, SRC(&left.get));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = StubDev->write(offset+BUFF_BYTES-2, 4, SRC(&right.get));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL ( o_buff.u16[0], OxDEAD.get );
	CU_ASSERT_EQUAL ( o_buff.u16[BUFF16_LENGTH-1], OxBEAF.get );

	/* this should overwrite the last two bytes of written left identifier
	 * and first two written bytes of right identifier. */
	err = StubDev->write(offset, BUFF_BYTES, SRC(o_buff.ptr));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = StubDev->read(offset-2, 4, DEST(&left.set));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = StubDev->read(offset+BUFF_BYTES-2, 4, DEST(&right.set));
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL ( left.as_u16._1, OxDEAD.get );
	CU_ASSERT_EQUAL ( left.as_u16._2, OxDEAD.get );
	CU_ASSERT_EQUAL ( right.as_u16._1, OxBEAF.get );
	CU_ASSERT_EQUAL ( right.as_u16._2, OxBEAF.get );
}

static void test_release(void)
{
	CU_ASSERT_EQUAL ( stub_memdev_release( StubDev ), E_GOOD );
}
