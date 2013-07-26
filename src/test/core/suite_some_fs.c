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
#include <flxlib.h>
#include <i7816.h>
#include <string.h>

#include <io/dev.h>

#include <common/test_macros.h>
#include <common/test_utils.h>

#include <fs/some/somefs.h>
#include <fs/some/data.h>
#include <fs/some/some_io.h>
#include <fs/some/data.h>

#include "stub_memdev.h"

PRIVATE int init_suite();
PRIVATE int clean_suite();

PRIVATE void test_df_struct_size(void);
PRIVATE void test_sb_clean(void);
PRIVATE void test_sb_read(void);
PRIVATE void test_sb_write(void);
PRIVATE void test_df_clean(void);
PRIVATE void test_fh_clean(void);
PRIVATE void test_df_handle_childs(void);
PRIVATE void test_df_write_and_read(void);
PRIVATE void test_fh_write_and_read(void);
PRIVATE void test_mkfs(void);

PRIVATE MemDev mdev;

static const struct test_case tc_arr[] = {
	TEST_CASE( test_df_struct_size, "size of some_df structure" ),
	TEST_CASE( test_sb_clean, "clean SomeSB" ),
	TEST_CASE( test_sb_write, "write SomeSB to memory device" ),
	TEST_CASE( test_sb_read, "read SomeSB from memory device" ),
	TEST_CASE( test_df_clean, "clean SomeDF" ),
	TEST_CASE( test_df_handle_childs, "adding entries to SomeDF" ),
	TEST_CASE( test_df_write_and_read, "write/read SomeDF to/from memory device" ),
	TEST_CASE( test_fh_clean, "clean SomeFH"),
	TEST_CASE( test_fh_write_and_read, "write/read SomeFH to/from memory device" ),
	TEST_CASE( test_mkfs, "mkfs on memory device" ),
};

int build_suite__somefs()
{
	INIT_BUILD_SUITE();
	CU_pSuite pSuite = NULL;

	CREATE_SUITE_OR_DIE("somefs core components", pSuite);
	ADD_TEST_CASES_OR_DIE(pSuite, tc_arr);

	return 0;
}

/* The suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
PRIVATE int
init_suite(void)
{
	return stub_memdev_init( &mdev );
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
PRIVATE int
clean_suite(void)
{
	return stub_memdev_release( &mdev );
}

PRIVATE void
sub_test_df_no_entries(SomeDF *df)
{
	int i;

	CU_ASSERT_PTR_NOT_NULL_FATAL ( df );

	for (i = 0; i < SOMEFS_MAX_DIR_ENTRIES; i++) {
		CU_ASSERT_EQUAL ( df->child[i].fid, 0 );
		CU_ASSERT_EQUAL ( df->child[i].addr, 0 );
	}
}

PRIVATE void
test_df_struct_size(void)
{
	CU_ASSERT_EQUAL( sizeof(struct some_df),
	                 SOMEFS_MAX_DIR_ENTRIES * sizeof(struct some_df_child));
}

PRIVATE void
test_df_clean(void)
{
	SomeDF df;
	memset( DEST(&df), 0xFF, sizeof(df));
	some_df_clean( &df );

	sub_test_df_no_entries( &df );
}

PRIVATE void
test_df_handle_childs(void)
{
	SomeDF df;
	err_t  err;

	struct some_df_child c1 = { .fid = 0x00ff, .addr = 128 };
	struct some_df_child c2 = { .fid = 0x001f, .addr = 180 };
	struct some_df_child hl = { .fid = 0xdead, .addr = 180 };

	some_df_clean(&df);

	err = some_df_put_child( NULL, c1.addr, c1.fid );
	CU_ASSERT_TRUE ( err & E_BAD_PARAM );

	err = some_df_put_child( &df, 0, c1.fid );
	CU_ASSERT_TRUE ( err & E_BAD_PARAM );

	err = some_df_put_child( &df, c1.addr, 0 );
	CU_ASSERT_TRUE ( err & E_BAD_PARAM );

	err = some_df_put_child( &df, c1.addr, c1.fid );
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = some_df_put_child( &df, c2.addr, c2.fid );
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = some_df_put_child( &df, c1.addr, c2.fid );
	CU_ASSERT_EQUAL ( err, E_FS_EXIST );

	err = some_df_put_child( &df, c2.addr, c1.fid );
	CU_ASSERT_EQUAL ( err, E_FS_EXIST );

	/* no hardlink support */
	err = some_df_put_child( &df, hl.addr, hl.fid );
	CU_ASSERT_EQUAL ( err, E_FS_EXIST );

	CU_ASSERT_EQUAL ( some_df_get_child(&df, c1.fid), c1.addr );
	CU_ASSERT_EQUAL ( some_df_get_child(&df, c2.fid), c2.addr );
	CU_ASSERT_EQUAL ( some_df_get_child(&df, hl.fid), 0 );
}

PRIVATE void
test_df_write_and_read(void)
{
	SomeDF    df_write;
	SomeDF    df_read;
	err_t     err;
	const u32 bytes = sizeof(df_write);

	some_df_clean(&df_write);
	some_df_clean(&df_read);

	err = E_GOOD;
	err |= some_df_put_child( &df_write, 1, 0x00ff );
	err |= some_df_put_child( &df_write, 2, 0x00fa );
	CU_ASSERT_EQUAL_FATAL ( err, E_GOOD );

	err = some_df_write(&mdev, 128, &df_write);
	CU_ASSERT_EQUAL ( err, E_GOOD );

	err = some_df_read(&mdev, 128, &df_read);
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL_BUFFER( SRC(&df_write), SRC(&df_read), bytes );
}

PRIVATE void
test_sb_clean(void)
{
	SomeSB sb;

	memset( DEST(&sb), 1, sizeof(sb) );

	some_sb_clean( &sb );

	CU_ASSERT_EQUAL( sb.magic, SOMEFS_MAGIC );
	CU_ASSERT_EQUAL( sb.mf_header, 0 );
	CU_ASSERT_EQUAL( sb.total_bytes, 0 );
	CU_ASSERT_EQUAL( sb.flags, 0 );
	CU_ASSERT_EQUAL( sb.next_free_addr, 0 );
}

PRIVATE void
test_sb_write(void)
{
	err_t err;
	SomeSB sb;

	some_sb_clean(&sb);

	sb.next_free_addr = sizeof(sb);
	sb.total_bytes = mdev.size;
	sb.mf_header = sizeof(sb);

	err = some_sb_write( NULL, &sb );
	CU_ASSERT_TRUE( err );

	err = some_sb_write( &mdev, NULL );
	CU_ASSERT_TRUE( err );

	err = some_sb_write( &mdev, &sb );
	CU_ASSERT_EQUAL( err, E_GOOD );

	/* test for invalid attributes */
	sb.next_free_addr = 1;
	err = some_sb_write( &mdev, &sb );
	CU_ASSERT_TRUE( err );

	sb.next_free_addr = sizeof(sb);

	sb.total_bytes = mdev.size - 1;
	err = some_sb_write( &mdev, &sb );
	CU_ASSERT_TRUE( err );
	sb.total_bytes = mdev.size;

	sb.mf_header -= 1;
	err = some_sb_write( &mdev, &sb );
	CU_ASSERT_TRUE( err );
	sb.mf_header += 1;

	sb.magic = 0;
	err = some_sb_write( &mdev, &sb );
	CU_ASSERT_TRUE ( err );
	sb.magic = SOMEFS_MAGIC;

	/* finally this should be fine */
	err = some_sb_write( &mdev, &sb );
	CU_ASSERT_EQUAL ( err, E_GOOD );
}

PRIVATE void
test_sb_read(void)
{
	err_t err;
	SomeSB sb;

	err = some_sb_read( &mdev, &sb );

	CU_ASSERT_EQUAL_FATAL( err, E_GOOD );

	CU_ASSERT_EQUAL( sb.magic, SOMEFS_MAGIC );
	CU_ASSERT_EQUAL( sb.total_bytes, mdev.size );
	CU_ASSERT_EQUAL( sb.flags, 0 );

	CU_ASSERT_NOT_EQUAL ( sb.mf_header, 0 );
	CU_ASSERT_NOT_EQUAL ( sb.next_free_addr, 0 );
}

PRIVATE void
test_fh_clean(void)
{
	SomeFH fh;

	memset(DEST(&fh), 0xff, sizeof(fh));
	some_fh_clean(&fh);

	CU_ASSERT_EQUAL ( fh.fdb, 0x00 );
	CU_ASSERT_EQUAL ( fh.sections, 0 );
	CU_ASSERT_EQUAL ( fh.sec_size, 0 );
	CU_ASSERT_EQUAL ( fh.data, 0 );
}

PRIVATE void
test_fh_write_and_read(void)
{
	err_t  err;
	SomeFH fh_write, fh_read, fh_clean;

	some_fh_clean(&fh_write);
	some_fh_clean(&fh_read);
	some_fh_clean(&fh_clean);

	fh_write.fdb = 0x00;

	err = some_fh_write(&mdev, 256, &fh_write);
	CU_ASSERT_NOT_EQUAL ( err, E_GOOD );

	fh_write.fdb = EF_WORKING | TRANSPARENT;
	err = some_fh_write(&mdev, 256, &fh_write);
	CU_ASSERT_EQUAL ( err, E_GOOD );

	fh_write.fdb = DF;
	err = some_fh_write(&mdev, 256, &fh_write);
	CU_ASSERT_EQUAL ( err, E_GOOD );

	/* force type check after read */
	err = some_fh_read(&mdev, 258, &fh_read);
	CU_ASSERT_TRUE ( err );
	/* do not leak any data */
	CU_ASSERT_EQUAL_BYTES ( fh_read, fh_clean );

	err = some_fh_read(&mdev, 256, &fh_read);
	CU_ASSERT_EQUAL ( err, E_GOOD );

	CU_ASSERT_EQUAL_BYTES (fh_write, fh_read);
}

PRIVATE void
test_mkfs(void)
{
	err_t  err;
	SomeSB sb;
	SomeFH fh;
	SomeDF df;

	err = somefs_mkfs(&mdev);
	CU_ASSERT_EQUAL( err, E_GOOD );

	/* check file system */
	err = some_sb_read(&mdev, &sb);
	CU_ASSERT_EQUAL( err, E_GOOD );

	err = some_fh_read(&mdev, sb.mf_header, &fh);
	CU_ASSERT_EQUAL( err, E_GOOD );
	CU_ASSERT_EQUAL( fh.fdb & MASK_TYPE, DF );
	CU_ASSERT_EQUAL( fh.sections, 1 );
	CU_ASSERT_EQUAL( fh.sec_size, sizeof(df) );
	CU_ASSERT_TRUE( sb.mf_header < fh.data );
	CU_ASSERT_NOT_EQUAL_FATAL( fh.data, 0 );

	err = some_df_read( &mdev, fh.data, &df );
	CU_ASSERT_EQUAL( err, E_GOOD );
	sub_test_df_no_entries( &df );
}
