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
#ifndef _TEST_UTILS_H
#define _TEST_UTILS_H

extern const union blob_u16 OxDEAD;
extern const union blob_u16 OxBEAF;
extern const union blob_u32 OxDEADBEAF;

/* provide some shared buffers */
extern u8 _buff_1[];
extern u8 _buff_2[];
extern const union buff_ux _ubuff_1;
extern const union buff_ux _ubuff_2;

enum BUFF_PROPERTIES {
	BUFF8_LENGTH  = 0x020000,        /* 128k * 1 byte = Flash block size */
	BUFF16_LENGTH = 0x010000,        /*  64k * 2 byte */
	BUFF32_LENGTH = 0x008000,        /*  32k * 4 byte */
	BUFF_BYTES  = BUFF8_LENGTH
};

/** Define a pointer union.
 *  Don't mess up with type casts every time */
union buff_ux {
	void *const ptr;
	u32  *const u32;
	u16  *const u16;
	u8   *const u8;
};

union blob_u16 {
	u16 set;
	const u16 get;
	u8  head8;
	struct __packed {
		u8 _1;
		u8 _2;
	} as_u8;
};

union blob_u32 {
	u32 set;
	const u32 get;
	u16 head16;
	u8  head8;
	struct {
		u16 _1;
		u16 _2;
	} as_u16;
	struct {
		u8 _1;
		u8 _2;
		u8 _3;
		u8 _4;
	} as_u8;
};

/**
 * @param[in] lhs
 * @param[in] rhs
 * @param[in] nbytes number of bytes to compare
 * @return number of sequenced equal bytes. A number equal to nbytes
 * indicates equality of buffers lhs and rhs.
 */
PUBLIC u32 sub_test_equals(buff8_t lhs, buff8_t rhs, u32 nbytes);

#define CU_ASSERT_EQUAL_BYTES(lhs, rhs) {       \
	if (sub_test_equals(SRC(&lhs),SRC(&rhs),sizeof(lhs)) < sizeof(lhs)) \
		CU_FAIL(#lhs" is not equal to "#rhs); }

#define CU_ASSERT_EQUAL_BUFFER(lhs, rhs, bytes) {   \
	if (sub_test_equals(lhs,rhs,bytes) < bytes) \
		CU_FAIL(#lhs"is not equal to "#rhs);}

#endif /* ----- end of macro protection ----- */
