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
#include <CUnit/Basic.h>

#include <apdu.h>
#include <apdu/commands.h>
#include <apdu/registry.h>

#include <common/test_macros.h>
#include <common/test_utils.h>

static int init_suite(void);
static int clean_suite(void);


static void test_validate_cmd__header_only(void);
static void test_validate_cmd__with_Le(void);
static void test_validate_cmd__with_ext_Le(void);
static void test_validate_cmd__with_Lc(void);
static void test_validate_cmd__with_ext_Lc(void);
static void test_validate_cmd__Lc_Le(void);
static void test_validate_cmd__ext_Lc_Le(void);
static void test_sizeof_apdu_header(void);
static void test_cmd_apdu__header_access(void);
static void test_apdu_registry_is_sorted(void);
static void test_apdu_resolve__select(void);
static void test_apdu_resolve__read_binary_b0(void);

static const struct test_case tc_arr[] = {
	TEST_CASE ( test_sizeof_apdu_header,        "size of 'struct apdu_header'" ),
	TEST_CASE ( test_cmd_apdu__header_access,   "access header fields of CmdAPDU" ),
	TEST_CASE ( test_validate_cmd__header_only, "validate command APDU: header only" ),
	TEST_CASE ( test_validate_cmd__with_Le,     "validate command APDU: one byte Le (only)" ),
	TEST_CASE ( test_validate_cmd__with_ext_Le, "validate command APDU: extended Le (only)" ),
	TEST_CASE ( test_validate_cmd__with_Lc,     "validate command APDU: one byte Lc (only)" ),
	TEST_CASE ( test_validate_cmd__with_ext_Lc, "validate command APDU: extended Lc (only)" ),
	TEST_CASE ( test_validate_cmd__Lc_Le,       "validate command APDU: one byte Lc and (ext) Le" ),
	TEST_CASE ( test_validate_cmd__ext_Lc_Le,   "validate command APDU: extended Lc and (ext) Le" ),
	TEST_CASE ( test_apdu_registry_is_sorted, "APDU registry is sorted" ),
	TEST_CASE ( test_apdu_resolve__select, "Instruction: SELECT" ),
	TEST_CASE ( test_apdu_resolve__read_binary_b0, "Instruction: READ BINARY (B0)" ),
};

/**
 *
 *
 */
int build_suite__apdu()
{
	INIT_BUILD_SUITE();
	CU_pSuite pSuite = NULL;

	CREATE_SUITE_OR_DIE("APDU registry", pSuite);
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

PRIVATE void
test_sizeof_apdu_header(void)
{
	CU_ASSERT_EQUAL (sizeof(struct apdu_header), 4);
}

PRIVATE void
test_cmd_apdu__header_access(void)
{
	static u8 buff[4] = { 0x80, 0x22, 0x01, 0x02 };
	CmdAPDU capdu = { .msg = buff, .length = sizeof(buff) };

	CU_ASSERT_EQUAL ( capdu.header->CLA, buff[0] );
	CU_ASSERT_EQUAL ( capdu.header->INS, buff[1] );
	CU_ASSERT_EQUAL ( capdu.header->P1,  buff[2] );
	CU_ASSERT_EQUAL ( capdu.header->P2,  buff[3] );
}

PRIVATE void
test_validate_cmd__header_only(void)
{
	static u8 buff[4] = { 0x80, 0x22, 0x00, 0x00 };
	CmdAPDU   capdu = { .msg = buff, .length = sizeof(buff) };
	err_t     err;

	err = apdu_validate_cmd( &capdu );

	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.length, 4);
	CU_ASSERT_EQUAL (capdu.Lc,     0);
	CU_ASSERT_EQUAL (capdu.Le,     0);

	CU_ASSERT_PTR_EQUAL (capdu.header, &buff);
	CU_ASSERT_PTR_EQUAL (capdu.data,   NULL);
}

PRIVATE void
test_validate_cmd__with_Le(void)
{
	static u8 buff[]  = { 0x80, 0x22, 0x00, 0x00, 0x04 };
	CmdAPDU   capdu = { .msg = buff, .length = sizeof(buff) };
	err_t     err;

	err = apdu_validate_cmd( &capdu );

	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.length, 5);
	CU_ASSERT_EQUAL (capdu.Lc,     0);
	CU_ASSERT_EQUAL (capdu.Le,     4);

	CU_ASSERT_PTR_EQUAL (capdu.header, &buff);
	CU_ASSERT_PTR_EQUAL (capdu.data,   NULL);

	/* special case: Le is 256 */
	buff[4] = 0x00;
	err = apdu_validate_cmd( &capdu );

	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.Le,     256);
}

PRIVATE void
test_validate_cmd__with_ext_Le(void)
{
	static u8 buff[] = { 0x80, 0x22, 0x00, 0x00, 0x00, 0x01, 0x04 };
	CmdAPDU   capdu = { .msg = buff, .length = sizeof(buff) };
	err_t     err;

	err = apdu_validate_cmd( &capdu );

	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.length, 7);
	CU_ASSERT_EQUAL (capdu.Lc,     0);
	CU_ASSERT_EQUAL (capdu.Le,     260);

	CU_ASSERT_PTR_EQUAL (capdu.header, &buff);
	CU_ASSERT_PTR_EQUAL (capdu.data,   NULL);

	buff[5] = buff[6] = 0x00;
	err = apdu_validate_cmd( &capdu );

	CU_ASSERT_NOT_EQUAL (err, E_GOOD);
}

PRIVATE void
test_validate_cmd__with_Lc(void)
{
	static u8 buff[]  = {
		/* header */
		0x80, 0x22, 0x00, 0x00,
		/* simple one byte Lc */
		0x04,
		/* command field */
		0x01, 0x02, 0x03, 0x04};
	CmdAPDU   capdu = { .msg = buff, .length = sizeof(buff) };
	err_t     err;

	err = apdu_validate_cmd( &capdu );

	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.length, 9);
	CU_ASSERT_EQUAL (capdu.Lc,     4);
	CU_ASSERT_EQUAL (capdu.Le,     0);

	CU_ASSERT_PTR_EQUAL (capdu.header, &buff);
	CU_ASSERT_PTR_EQUAL (capdu.data,   &buff[5]);

	buff[4] = 0x00;
	err = apdu_validate_cmd( &capdu );

	CU_ASSERT_NOT_EQUAL (err, E_GOOD);
}

PRIVATE void
test_validate_cmd__with_ext_Lc(void)
{
	static u8 buff[] = {
		/* header */
		0x80, 0x22, 0x00, 0x00,
		/* extended Lx */
		0x00, 0x00, 0x04,
		/* data body */
		0x01, 0x02, 0x03, 0x04 };
	CmdAPDU   capdu = { .msg = buff, .length = sizeof(buff) };
	err_t     err;

	err = apdu_validate_cmd( &capdu );

	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.length, 11);
	CU_ASSERT_EQUAL (capdu.Lc,     4);
	CU_ASSERT_EQUAL (capdu.Le,     0);

	CU_ASSERT_PTR_EQUAL (capdu.header, &buff);
	CU_ASSERT_PTR_EQUAL (capdu.data,   &buff[7]);

	buff[5] = buff[6] = 0x00;
	err = apdu_validate_cmd( &capdu );

	CU_ASSERT_NOT_EQUAL (err, E_GOOD);
}

PRIVATE void
test_validate_cmd__Lc_Le(void)
{
	static u8 buff[] = {
		/* header */
		0x80, 0x22, 0x00, 0x00,
		/* normal one byte Lc */
		0x04,
		/* data body */
		0x01, 0x02, 0x03, 0x04,
		/* Le */
		0xFF, 0x01,
		/* additional byte to test invalid length */
		0x00
	};
	CmdAPDU   capdu = { .msg = buff, .length = sizeof(buff) };
	err_t     err;

	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_EQUAL (err,          E_APDU_LC);

	capdu.length--;

	/* test two byte Le */
	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.length, 11);
	CU_ASSERT_EQUAL (capdu.Lc,     4);
	CU_ASSERT_EQUAL (capdu.Le,     0xFF01);

	CU_ASSERT_PTR_EQUAL (capdu.header, &buff);
	CU_ASSERT_PTR_EQUAL (capdu.data,   &buff[5]);

	/* test one byte Le */
	capdu.length--;
	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.length, 10);
	CU_ASSERT_EQUAL (capdu.Lc,     4);
	CU_ASSERT_EQUAL (capdu.Le,     0xFF);

	CU_ASSERT_PTR_EQUAL (capdu.header, &buff);
	CU_ASSERT_PTR_EQUAL (capdu.data,   &buff[5]);

	/* Le MUST NOT be zero */
	buff[9] = buff[10] = 0;
	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.Le,     256);
	/* switch back to extended Le */
	capdu.length++;
	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_EQUAL (ecode(err),   E_APDU_LE);

	/* Lc MUST NOT be zero */
	buff[4] = 0x00;
	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_TRUE (err);
}

PRIVATE void
test_validate_cmd__ext_Lc_Le(void)
{
	static u8 buff[] = {
		/* header */
		0x80, 0x22, 0x00, 0x00,
		/* extended Lc */
		0x00, 0x00, 0x04,
		/* data body */
		0x01, 0x02, 0x03, 0x04,
		/* Le */
		0xFF, 0x01,
		/* additional byte to test invalid length */
		0x00
	};
	CmdAPDU   capdu = { .msg = buff, .length = sizeof(buff) };
	err_t     err;

	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_EQUAL (ecode(err),   E_APDU_LC);

	capdu.length--;

	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.length, 13);
	CU_ASSERT_EQUAL (capdu.Lc,     4);
	CU_ASSERT_EQUAL (capdu.Le,     0xFF01);

	CU_ASSERT_PTR_EQUAL (capdu.header, &buff);
	CU_ASSERT_PTR_EQUAL (capdu.data,   &buff[7]);

	/* test one byte Le */
	capdu.length--;
	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.length, 12);
	CU_ASSERT_EQUAL (capdu.Lc,     4);
	CU_ASSERT_EQUAL (capdu.Le,     0xFF);

	CU_ASSERT_PTR_EQUAL (capdu.header, &buff);
	CU_ASSERT_PTR_EQUAL (capdu.data,   &buff[7]);

	/* Le MUST NOT be zero */
	buff[11] = buff[12] = 0;
	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_EQUAL (err,          E_GOOD);
	CU_ASSERT_EQUAL (capdu.Le,    256);
	/* switch back to extended Le */
	capdu.length++;
	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_EQUAL (ecode(err),   E_APDU_LE);

	buff[11] = buff[12] = 0x01;
	buff[5]  = buff[6]  = 0x00;
	err = apdu_validate_cmd( &capdu );
	CU_ASSERT_TRUE (err);
}

PRIVATE void
test_apdu_registry_is_sorted(void)
{
	u8 i;

	if (apdu_registry_size < 2) {
		CU_PASS ( "registry size is less than 2" );
		return;
	}

	for (i = 1; i < apdu_registry_size; i++)
		if (apdu_registry[i].ins <= apdu_registry[i-1].ins)
			CU_FAIL_FATAL ( "registry array is not sorted" );
}

PRIVATE void
test_apdu_resolve__select(void)
{
	u8 apdu[4]  = {0};
	CmdAPDU capdu = { .msg = apdu, .length = sizeof(apdu) };
	/* we want write access to header field */
	struct apdu_header *header = (struct apdu_header *) apdu;
	fp_handle_cmd_apdu cmd;

	header->INS= 0xA4;
	header->P2 = 0x00;

	/* SELECT BY FID:
	 * 00 00 00 xx */
	header->P1 = 0x00;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL (cmd, cmd_select__by_fid);
	header->P1 = 0x01;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL (cmd, cmd_select__by_fid);
	header->P1 = 0x02;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL (cmd, cmd_select__by_fid);
	header->P1 = 0x03;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL (cmd, cmd_select__by_fid);

	/* select by dfname:
	 * 00 00 01 00 */
	header->P1 = 0x04;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL (cmd, cmd_select__by_dfname);
	header->P1 = 0x05;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL (cmd, cmd__wrong_p1_p2);

	/* select by path
	 * 00 00 10 0x */
	header->P1 = 0x08;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL (cmd, cmd_select__by_path);
	header->P1 = 0x09;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL (cmd, cmd_select__by_path);


	header->P1 = 0x0a;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL (cmd, cmd__wrong_p1_p2);
	header->P1 = 0x18;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL (cmd, cmd__wrong_p1_p2);
}

PRIVATE void
test_apdu_resolve__read_binary_b0(void)
{
	u8 apdu[4]  = {0};
	CmdAPDU capdu = { .msg = apdu, .length = sizeof(apdu) };
	/* we want write access to header field */
	struct apdu_header *header = (struct apdu_header *) apdu;
	fp_handle_cmd_apdu cmd;

	header->INS = 0xB0;
	header->P1  = 0x00;
	header->P2  = 0x00;

	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL ( cmd, cmd_read_binary_b0__from_current_EF );

	header->P1  = 0x00;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL ( cmd, cmd_read_binary_b0__from_current_EF );

	header->P1  = 0x0F;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL ( cmd, cmd_read_binary_b0__from_current_EF );

	header->P1  = 0x7F;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL ( cmd, cmd_read_binary_b0__from_current_EF );

	header->P1  = 0x8F;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL ( cmd, cmd_read_binary_b0__from_short_EF_id );

	header->P1  = 0x80;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL ( cmd, cmd_read_binary_b0__from_short_EF_id );

	header->P1  = 0x90;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL ( cmd, cmd_read_binary_b0__from_short_EF_id );

	header->P1  = 0xB0;
	cmd = apdu_get_cmd_handler(&capdu);
	CU_ASSERT_PTR_EQUAL ( cmd, cmd__wrong_p1_p2 );
}
