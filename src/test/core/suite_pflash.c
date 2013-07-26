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

/* Xilinx flash API */
#include <xilflash.h>

#include <const.h>
#include <flash_fpga.h>
#include <types.h>

#include <common/test_macros.h>
#include <common/test_utils.h>

#include "suites.h"

static int init_suite(void);
static int clean_suite(void);

static void test_self_check(void);
static void test_api_reset(void);
static void test_api_unlock(void);
static void test_api_ready(void);
static void test_api_read(void);
static void test_erase_block(void);
static void test_erase_impact(void);
static void test_write_one_byte(void);
static void test_write_buffer_aligned(void);
static void test_write_buffer_across(void);
static void test_write_zero_byte(void);
static void test_total_size(void);
static void test_try_to_overwrite(void);
static void test_max_block_size(void);
static void sub_test_io_buffers(void);

static XFlash Flash;
static XFlashGeometry *GeomPtr;

static const struct test_case tc_arr[] = {
	TEST_CASE( test_self_check, "suite self check" ),
	TEST_CASE( test_api_ready, "flash device isReady" ),
	TEST_CASE( test_total_size,"total size"),
	TEST_CASE( test_max_block_size, "max block size"),
	TEST_CASE( test_api_reset, "Xilflash API - reset" ),
	TEST_CASE( test_api_unlock, "Xilflash API - unlock" ),
	TEST_CASE( test_api_read,  "Xilflash API - read" ),
	TEST_CASE( test_erase_block, "Xilflash API - erase" ),
	TEST_CASE( test_write_one_byte, "Xilflash API - write single byte"),
	TEST_CASE( test_write_buffer_aligned, "write buffer block aligned" ),
	TEST_CASE( test_write_buffer_across, "write buffer across blocks" ),
	TEST_CASE( test_write_zero_byte, "write integrity" ),
	TEST_CASE( test_erase_impact, "validate erase impact" ),
	TEST_CASE( test_try_to_overwrite, "overwrite behaviour")
};

enum {
	USED_BUFF32_LENGTH = 0x0100,
	USED_BUFF_BYTES    = 0x0400
};

/* input buffer - used to read from flash */
static u32 *i_buff;
/* output buffer - content is written to flash */
static u32 *o_buff;
/* do not use the whole buffer for this test suite.
 * But we have to check that 'BUFF32_LENGTH' is greater */

/**
 *
 */
int build_suite__flash()
{
	INIT_BUILD_SUITE();
	CU_pSuite suite = NULL;

	CREATE_SUITE_OR_DIE("Parallel Flash - Xilinx API", suite);
	ADD_TEST_CASES_OR_DIE(suite, tc_arr);

	return 0;
}

/* The suite initialization function.
 *
 * An initialized XFlash structure is mandatory for this test suite.
 */
int init_suite(void)
{
	int i;
	int ret_code;

	i_buff = _ubuff_1.u32;
	o_buff = _ubuff_2.u32;

	if (USED_BUFF32_LENGTH > BUFF32_LENGTH)
		return 2;
	/* fill output buffer with '0xdeadbeaf' */
	for (i = 0; i < USED_BUFF32_LENGTH; i++) {
		o_buff[i] = OxDEADBEAF.get;
	}

	ret_code = XFlash_Initialize(&Flash,
			FLASH_BASEADDR,
			FLASH_BUS_WIDTH, 0);

	if (ret_code == XST_SUCCESS) {
		GeomPtr = &(Flash.Geometry);
		return 0;
	}

	return 1;
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Test cases                                                                */
/* -------------------------------------------------------------------------- */

static void test_self_check(void) {
	CU_ASSERT_NOT_EQUAL ( i_buff, o_buff );
	CU_ASSERT_EQUAL ( USED_BUFF32_LENGTH * 4, USED_BUFF_BYTES );
	CU_ASSERT_EQUAL ( o_buff[0], OxDEADBEAF.get );
	CU_ASSERT_EQUAL ( o_buff[USED_BUFF32_LENGTH-1], OxDEADBEAF.get );
}
/* This test case should run first.
 * It implicitly validates a successful XFlash_Initialize(...) */
static void test_api_ready()
{
	CU_ASSERT_TRUE_FATAL (XFlash_IsReady(&Flash));
	return;
}

/*
 * Reset the Flash Device. This clears the Status registers and puts
 * the device in Read mode.
 */
static void test_api_reset(void)
{
	int err_code;
	err_code = XFlash_Reset(&Flash);
	CU_ASSERT_EQUAL (err_code, XST_SUCCESS);
}

/*
 * Perform an unlock operation before the erase operation for the Intel
 * Flash. The erase operation will result in an error if the block is
 * locked.
 */
static void test_api_unlock(void)
{
	int err_code;
	CU_ASSERT_TRUE (
		(Flash.CommandSet == XFL_CMDSET_INTEL_STANDARD) ||
		(Flash.CommandSet == XFL_CMDSET_INTEL_EXTENDED));
	err_code = XFlash_Unlock(&Flash, 0, FLASH_SIZE);
	CU_ASSERT_EQUAL (err_code, XST_SUCCESS);
}

static void test_total_size(void)
{
	CU_ASSERT_TRUE ( FLASH_SIZE <= Flash.Geometry.DeviceSize );
	return;
}

static void test_max_block_size(void)
{
	u32 i;
	u32 n = Flash.Geometry.NumEraseRegions;
	u32 m = 0;

	XFlashGeometry *GeomPtr = &(Flash.Geometry);

	for (i = 0; i < n; i++) {
		if (GeomPtr->EraseRegion[i].Size > m)
			m = GeomPtr->EraseRegion[i].Size;
	}

	CU_ASSERT_EQUAL (m, FLASH_BLOCK_SIZE_MAX);

	return;
}

static void test_api_read(void)
{
	int err_code;
	XFlash InvalidFlash;
	u32 n;
	u32 nbytes = sizeof (n);

	CU_ASSERT_TRUE_FATAL (USED_BUFF_BYTES > 8);

	err_code = XFlash_Read(&InvalidFlash, 0, USED_BUFF_BYTES, DEST(i_buff));
	CU_ASSERT_NOT_EQUAL (err_code, XST_SUCCESS);

	/* arbitrary read */
	err_code = XFlash_Read(&Flash, 0, USED_BUFF_BYTES, DEST(i_buff));
	CU_ASSERT_EQUAL (err_code, XST_SUCCESS);

	/* read bytes 4 to 7 from same address as above */
	err_code = XFlash_Read(&Flash, 4, nbytes, DEST(&n) );
	CU_ASSERT_EQUAL (err_code, XST_SUCCESS);

	/* did we get the same data */
	CU_ASSERT_EQUAL ( n, i_buff[1]);

	/* read very last four bytes */
	err_code = XFlash_Read(&Flash, FLASH_SIZE - 4, 4, DEST(&n));
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );

	/* Source address starts within the addressable area.
	 * XilFlash library should not report any error. */
	err_code = XFlash_Read(&Flash, FLASH_SIZE - 2, 4, DEST(&n));
	CU_ASSERT_EQUAL( err_code, XST_SUCCESS );

	/* Source address exceeds the addressable area.
	 * XilFlash library must not succeed. */
	err_code = XFlash_Read(&Flash, FLASH_SIZE, 4, DEST(&n));
	CU_ASSERT_NOT_EQUAL( err_code, XST_SUCCESS );

	/* read across block boundary */
	n = Flash.Geometry.EraseRegion[0].Size - USED_BUFF32_LENGTH + 8;
	err_code = XFlash_Read(&Flash, n, USED_BUFF_BYTES, DEST(i_buff));
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );

	return;
}

/* Execute some erasures with different address ranges,
 * i.e., complete block, partial block or across blocks */

/*  */
static void test_erase_block(void)
{
	int err_code;

	/* execute erase command on the first byte */
	err_code = XFlash_Erase(&Flash, 0, GeomPtr->EraseRegion[0].Size - 1);
	CU_ASSERT_NOT_EQUAL ( err_code, XFLASH_ADDRESS_ERROR );
	CU_ASSERT_EQUAL (err_code, XST_SUCCESS);
}

static void test_erase_impact(void)
{
	int err_code;
	union blob_u32 _deadbeaf;
	u32 bsize = GeomPtr->EraseRegion[0].Size;

	/* clean environment */
	err_code = XFlash_Erase(&Flash, 0, bsize - 1);
	CU_ASSERT_EQUAL (err_code, XST_SUCCESS);

	err_code = XFlash_Write(&Flash, 0, 4, SRC(&OxDEADBEAF));
	CU_ASSERT_EQUAL (err_code, XST_SUCCESS);

	err_code = XFlash_Erase(&Flash, 1, 2);
	CU_ASSERT_EQUAL (err_code, XST_SUCCESS);

	err_code = XFlash_Read(&Flash, 0, 4, DEST(&_deadbeaf));
	CU_ASSERT_EQUAL (err_code, XST_SUCCESS);

	CU_ASSERT_EQUAL ( _deadbeaf.get, 0xFFFFFFFF);
}

/* Write a single byte anywhere on the Flash device.
 * This test does just check exit code of XilFlash write command */
static void test_write_one_byte(void)
{
	int err_code;
	/* write anywhere one byte */
	err_code = XFlash_Write (&Flash, 12, 1, SRC(&OxDEAD));
	CU_ASSERT_NOT_EQUAL ( err_code, XFLASH_ADDRESS_ERROR );
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );
}

/* Writing to flash memory is limited to flipping bits from one to zero.
 * This test overwrites two bytes and validates the expected behaviour. */
static void test_try_to_overwrite(void)
{
	int err_code;
	u16 val16;
	u32 offset = 0;

	err_code = XFlash_Erase(&Flash, offset, 2);
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );

	err_code = XFlash_Write(&Flash, offset, 2, SRC(&OxDEAD));
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );

	err_code = XFlash_Read(&Flash, offset, 2, DEST(&val16));
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );
	CU_ASSERT_EQUAL ( val16, OxDEAD.get );

	err_code = XFlash_Write(&Flash, offset, 2, SRC(&OxBEAF));
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );

	err_code = XFlash_Read(&Flash, offset, 2, DEST(&val16));
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );
	CU_ASSERT_EQUAL ( val16, OxDEAD.get & OxBEAF.get );
}

/* write the whole buffer into one flash block without any error */
static void test_write_buffer_aligned(void)
{
	int err_code;
	u32 offset;

	/* we want to write into second block of first region */
	offset = GeomPtr->EraseRegion[0].Size;

	/* this test depends on a successful erasure */
	err_code = XFlash_Erase(&Flash, offset, USED_BUFF_BYTES);
	CU_ASSERT_EQUAL_FATAL ( err_code, XST_SUCCESS );

	/* write the buffer without any error */
	err_code = XFlash_Write(&Flash, offset, USED_BUFF_BYTES, SRC(o_buff));
	CU_ASSERT_NOT_EQUAL ( err_code, XFLASH_ADDRESS_ERROR );
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );

	err_code = XFlash_Read(&Flash, offset, USED_BUFF_BYTES, DEST(i_buff));
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );

	sub_test_io_buffers();
}

// XXX consider export as a common function ~ Testing array content on equality
static void sub_test_io_buffers()
{
	int i;
	for (i = 0; i < USED_BUFF32_LENGTH; i++)
		if (i_buff[i] != o_buff[i]) {
			CU_FAIL ("data of i_buff != o_buff");
			break;
		}
	return;
}

/* Write a buffer across two blocks without any error.
 * This test case start in the second absolut block */
static void test_write_buffer_across(void)
{
	int err_code;
	/* Calculate an offset address within the second block */
	const u32 offset = ((2*GeomPtr->EraseRegion[0].Size)
			     - USED_BUFF_BYTES + 16);

	/* expect more than one block in the first erase region */
	CU_ASSERT_TRUE_FATAL ( GeomPtr->EraseRegion[0].Number > 1 );
	CU_ASSERT_TRUE_FATAL ( GeomPtr->EraseRegion[0].Size > USED_BUFF_BYTES );

	/* this test depends on a successful erasure */
	err_code = XFlash_Erase(&Flash, offset, USED_BUFF_BYTES);
	CU_ASSERT_NOT_EQUAL ( err_code, XFLASH_ADDRESS_ERROR );
	CU_ASSERT_EQUAL_FATAL ( err_code, XST_SUCCESS );

	err_code = XFlash_Write(&Flash, offset, USED_BUFF_BYTES, SRC(o_buff));
	CU_ASSERT_NOT_EQUAL ( err_code, XFLASH_ADDRESS_ERROR );
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );
}

static void test_write_zero_byte(void)
{
	int err_code;

	union blob_u16 before;
	union blob_u16 after;

	const u8  zero = 0x00;
	const u32 from_start = 0;
	const u32 nbytes = sizeof(before);

	/* this test case is designed for a union group of 16 bit size */
	if (nbytes != 2)
		CU_FAIL ("size of union type 'blob_u16' "
		         "is not two bytes as expected");

	/* read first two bytes */
	err_code = XFlash_Read(&Flash, from_start, nbytes, DEST(&before));
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );

	/* set first byte to zero */
	err_code = XFlash_Write(&Flash, from_start, 1, SRC(&zero));
	CU_ASSERT_NOT_EQUAL ( err_code, XFLASH_ADDRESS_ERROR );
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );

	/* read first two bytes again */
	err_code = XFlash_Read(&Flash, from_start, nbytes, DEST(&after));
	CU_ASSERT_EQUAL ( err_code, XST_SUCCESS );

	if (before.head8 != zero) {
		CU_ASSERT_EQUAL ( after.head8, zero );
	} else {
		CU_PASS ("do not check write integrity due to equality");
	}
	/* has the write command overwritten the successor byte? */
	CU_ASSERT_EQUAL ( after.as_u8._2, before.as_u8._2 );

	return;
}

