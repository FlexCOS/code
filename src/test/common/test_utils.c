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

#include "test_utils.h"

const union blob_u16 OxDEAD = { .as_u8 = {._1 = 0xde, ._2 = 0xad} };
const union blob_u16 OxBEAF = { .as_u8 = {._1 = 0xbe, ._2 = 0xaf} };
const union blob_u32 OxDEADBEAF = { .as_u8 = {
	._1 = 0xde,
	._2 = 0xad,
	._3 = 0xbe,
	._4 = 0xaf }};

/* XXX this is a malloc work around */
u8 _buff_1[BUFF_BYTES];
u8 _buff_2[BUFF_BYTES];
/* this could probably be handy!? don't know... */
const union buff_ux _ubuff_1 = { .u8 = _buff_1 };
const union buff_ux _ubuff_2 = { .u8 = _buff_2 };



PUBLIC u32
sub_test_equals(buff8_t lhs, buff8_t rhs, u32 bytes)
{
	u32 i;
	for (i = 0; i < bytes; i++) {
		CU_ASSERT_EQUAL ( lhs[i], rhs[i] );
		if (lhs[i] != rhs[i]) break;
	}

	return i;
}


