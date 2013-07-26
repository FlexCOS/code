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

#include <common/list.h>
#include <tlv.h>

#include <common/test_macros.h>
#include <common/test_utils.h>

static int init_suite(void);
static int clean_suite(void);

static void test_ber_primitive(void);
static void test_ber_long_tags(void);
static void test_invalid_tag_field(void);
static void test_invalid_length_field(void);
static void test_ber_nesting_simple(void);
static void test_ber_nesting_complex(void);


static const struct test_case tc_arr[] = {
	TEST_CASE( test_ber_primitive, "decode primitive BER-TLV objects" ),
	TEST_CASE( test_ber_long_tags, "decode BER tags of multiple bytes" ),
	TEST_CASE( test_invalid_tag_field,    "catch invalid tag fields" ),
	TEST_CASE( test_invalid_length_field, "catch invalid length fields" ),
	TEST_CASE( test_ber_nesting_simple,   "one time tlv nesting" ),
	TEST_CASE( test_ber_nesting_complex,  "complex nested tlv structure" )
};



/**
 *
 *
 */
int build_suite__tlv_parser()
{
	INIT_BUILD_SUITE();
	CU_pSuite pSuite = NULL;

	CREATE_SUITE_OR_DIE("BER-TLV parser", pSuite);
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

static enum Tlv_Parse_Cmd
test_ber_primitive__visit(const struct tlv_parse_ctx *tlv, void *opaque)
{
	u8 visited = *(u8 *)opaque;

	switch (tlv->tag) {
	case 0x81:
		CU_ASSERT_TRUE ( visited ^ 0x01 );
		CU_ASSERT_EQUAL( tlv->length, 0x02 );
		CU_ASSERT_TRUE ( list_empty(&tlv->nesting) );

		visited ^= 0x01;
		break;
	case 0x82:
		CU_ASSERT_TRUE ( visited ^ 0x02 );
		CU_ASSERT_EQUAL( tlv->length, 0x03 );
		CU_ASSERT_TRUE ( list_empty(&tlv->nesting) );

		visited ^= 0x02;
		break;
	case 0x84:
		CU_ASSERT_TRUE ( visited ^ 0x04 );
		CU_ASSERT_EQUAL( tlv->length, 0x12ff );
		CU_ASSERT_TRUE ( list_empty(&tlv->nesting) );

		visited ^= 0x04;
		break;
	default:
		CU_FAIL ("parse error");
		return STOP;
	}

	* (u8 *)opaque = visited;

	return NEXT;
}

static void
test_ber_primitive(void)
{
	err_t err;
	u8 *data = _ubuff_1.u8;
	u8 map  = 0x00;

	data[0] = 0x81;  /* one tag byte */
	data[1] = 0x02;  /* one value byte */
	data[2] = 0xD1;  /* dummy data to check against correct positioning */
	data[3] = 0xD2;

	data[4] = 0x82;
	data[5] = 0x81;  /* length field with one extra byte */
	data[6] = 0x03;
	data[7] = 0xE1;
	data[8] = 0xE2;
	data[9] = 0xE3;

	data[10] = 0x84;
	data[11] = 0x82; /* length field with two extra bytes */
	data[12] = 0x12;
	data[13] = 0xff;
	data[14] = 0xF1;
	data[15] = 0xF2;

	err = tlv_parse_ber(data, 14 + 0x12ff, test_ber_primitive__visit, &map);

	CU_ASSERT_EQUAL( err, E_GOOD );
	CU_ASSERT_EQUAL( map, 0x07 );
}

static enum Tlv_Parse_Cmd
__never_visit(const struct tlv_parse_ctx *ctx, void *ignore)
{
	CU_FAIL("unexpected visit");
	return NEXT;
}

static enum Tlv_Parse_Cmd
__long_tags__visit(const struct tlv_parse_ctx *tlv, void *visited)
{
	switch (tlv->tag) {
	case 0x9F1F:
	case 0x9F810F:
	case 0x9F8100:
		CU_ASSERT_EQUAL( tlv->length, 0x00 );
		*(bool *) visited = true;
		break;
	default:
		CU_FAIL ("unexpected tag value");
	}

	return NEXT;
}

static void
test_ber_long_tags(void)
{
	err_t err;
	u8 *data = _ubuff_1.u8;
	bool visited = false;

	data[0] = 0x9F;
	data[1] = 0x1F;
	data[2] = 0x00;

	err = tlv_parse_ber(data, 3, __long_tags__visit, &visited);
	CU_ASSERT_EQUAL( err, E_GOOD );
	CU_ASSERT_TRUE ( visited );

	data[0] = 0x9F;
	data[1] = 0x81;
	data[2] = 0x0F;
	data[3] = 0x00;

	err = tlv_parse_ber(data, 4, __long_tags__visit, &visited);
	CU_ASSERT_EQUAL( err, E_GOOD );
	CU_ASSERT_TRUE ( visited );

	data[0] = 0x9F;
	data[1] = 0x81;
	data[2] = 0x00;
	data[3] = 0x00;

	err = tlv_parse_ber(data, 4, __long_tags__visit, &visited);
	CU_ASSERT_EQUAL( err, E_GOOD );
	CU_ASSERT_TRUE ( visited );

}

static void
test_invalid_tag_field(void)
{
	err_t err;
	u8 *data = _ubuff_1.u8;

	data[0] = 0x00; /* 0x00 is invalid for first byte of a tag field */
	data[1] = 0x00;

	err = tlv_parse_ber(data, 2, __never_visit, NULL);
	CU_ASSERT_EQUAL( err, E_TLV_TAG );


	data[0] = 0x9F;
	data[1] = 0x00; /* BER-TAG must not be zero. */

	err = tlv_parse_ber(data, 2, __never_visit, NULL);
	CU_ASSERT_EQUAL( err, E_TLV_TAG );


	data[0] = 0x9F;
	data[1] = 0x80; /* invalid for the second byte */
	data[2] = 0x00; /* <- but parser might error here, as he could continue
	                 *    with a further tag field byte. */

	err = tlv_parse_ber(data, 3, __never_visit, NULL);
	CU_ASSERT_EQUAL( err, E_TLV_TAG );


	data[0] = 0x9F;
	data[1] = 0x80; /* bits 1 to 7 of first subsequent byte must not be 0 */
	data[2] = 0x01; /* <- thats why we test both cases (see previous test)*/
	data[3] = 0x01;

	err = tlv_parse_ber(data, 4, __never_visit, NULL);
	CU_ASSERT_EQUAL( err, E_TLV_TAG );

	data[0] = 0x9F;
	data[1] = 0x1E; /* invalid, cause: representable with one tag byte. */
	data[2] = 0x01;
	data[3] = 0xD1;

	err = tlv_parse_ber(data, 4, __never_visit, NULL);
	CU_ASSERT_EQUAL( err, E_TLV_TAG );


	data[0] = 0x9F;
	data[1] = 0x80;
	data[2] = 0x01;
	data[3] = 0x00;

	err = tlv_parse_ber(data, 4, __never_visit, NULL);
	CU_ASSERT_EQUAL( err, E_TLV_TAG );

	data[0] = 0x9F;
	data[1] = 0x81;
	data[2] = 0x80;
	data[3] = 0x01;
	data[4] = 0x00;

	err = tlv_parse_ber(data, 5, __never_visit, NULL);
	CU_ASSERT_EQUAL( err, E_TLV_TAG );
}

static void
test_invalid_length_field(void)
{
	err_t err;
	u8 *data = _ubuff_1.u8;

	data[0] = 0x81;
	data[1] = 0x80;
	data[2] = 0x00;
	data[3] = 0x00;
	data[4] = 0x00;
	data[5] = 0x00;
	data[6] = 0x00;


	err = tlv_parse_ber(data, 7, __never_visit, NULL);
	CU_ASSERT_EQUAL( err, E_TLV_LEN );

	data[1] = 0x89;
	err = tlv_parse_ber(data, 7, __never_visit, NULL);
	CU_ASSERT_EQUAL( err, E_TLV_LEN );

	data[1] = 0x85;
	err = tlv_parse_ber(data, 7, __never_visit, NULL);
	CU_ASSERT_EQUAL( err, E_TLV_LEN );

	data[1] = 0x92;
	err = tlv_parse_ber(data, 7, __never_visit, NULL);
	CU_ASSERT_EQUAL( err, E_TLV_LEN );
}


static enum Tlv_Parse_Cmd
__nesting_simple__visit(const struct tlv_parse_ctx *tlv, void *opaque)
{
	struct tlv_nesting *parent;

	if (tlv->tag == 0x81) {
		if (list_empty(&tlv->nesting))
			return STOP;

		parent = list_first_entry(&tlv->nesting, struct tlv_nesting, list);
		if (parent->tag == 0x61)
			*(bool *) opaque = true;
		else
			CU_FAIL( "No nesting reference to parent 0x61" );
	}

	return STEP_INTO;
}

static void
test_ber_nesting_simple(void)
{
	err_t err;
	u8    *data = _ubuff_1.u8;
	bool  success;

	data[0] = 0x61;
	data[1] = 0x03;
	data[2] = 0x81;
	data[3] = 0x01;
	data[4] = 0xD1;

	err = tlv_parse_ber(data, 5, __nesting_simple__visit, &success);
	CU_ASSERT_EQUAL( err, E_GOOD );
	CU_ASSERT_TRUE ( success );
}

static enum Tlv_Parse_Cmd
__nesting_complex__visit(const struct tlv_parse_ctx *tlv, void *opaque)
{
	struct tlv_nesting *p1 = NULL;

	if (!list_empty(&tlv->nesting))
		p1 = list_first_entry(&tlv->nesting, struct tlv_nesting, list);


	switch (tlv->tag) {
	case 0x61:
	case 0xC2:
		CU_ASSERT_PTR_NULL(p1);
		break;
	case 0x88:
	case 0xA1:
		if (p1) {
			CU_ASSERT_EQUAL( p1->tag, 0x61 );
		}
		else
			CU_FAIL("nesting error A1/88");
		break;
	case 0xB1:
	case 0xC1:
		if (p1) {
			CU_ASSERT_EQUAL( p1->tag, 0xA1 );
		}
		else
			CU_FAIL("nesting error B1/C1");
		break;
	case 0xDF1F:
	case 0xDF2F:
		if (p1) {
			CU_ASSERT_EQUAL( p1->tag, 0xB1 );
		}
		else
			CU_FAIL("nesting error DF{12}F");
		break;
	default:
		CU_FAIL("parse error");
		return STOP;
	}

	return STEP_INTO;
}

static void
test_ber_nesting_complex(void)
{
	err_t err;
	u8    *data = _ubuff_1.u8;

	data[0] = 0x61;
	data[1] = 0x10;
		data[2] = 0x88;
		data[3] = 0x01;
		data[4] = 0xD8;

		data[5] = 0xA1;
		data[6] = 0x0B;
			data[7] = 0xB1;
			data[8] = 0x07;
				data[9] = 0xDF;
				data[10] = 0x1F;
				data[11] = 0x01;
				data[12] = 0xDD;

				data[13] = 0xDF;
				data[14] = 0x2F;
				data[15] = 0x00;

			data[16] = 0xC1;
			data[17] = 0x00;


	data[18] = 0xC2;
	data[19] = 0x01;
	data[20] = 0x00;

	err = tlv_parse_ber(data, 21, __nesting_complex__visit, NULL);
	CU_ASSERT_EQUAL( err, E_GOOD );
}
