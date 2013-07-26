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

#include "mock_terminal.h"

PRIVATE int init_suite(void);
PRIVATE int clean_suite(void);

PRIVATE void test_cmd__get_challenge(void);

PRIVATE struct test_case tc_arr[] = {
	TEST_CASE( test_cmd__get_challenge, "cmd: get challenge" )
};

PRIVATE u8        recv_buff[1024];
PRIVATE const u16 recv_max = sizeof(recv_buff);


PUBLIC int
build_suite__flexcos()
{
	INIT_BUILD_SUITE();
	CU_pSuite pSuite = NULL;

	CREATE_SUITE_OR_DIE("FlexCOS", pSuite);
	ADD_TEST_CASES_OR_DIE(pSuite, tc_arr);

	return 0;
}

PRIVATE int
init_suite()
{
	return 0;
}

PRIVATE int
clean_suite()
{
	return 0;
}

PRIVATE void
test_cmd__get_challenge(void)
{
	u8 cmd[] = { 0x08, 0x84, 0x00, 0x00, 0x08 };
	u8 cmd_length = sizeof(cmd);
	u16 l;

	/* send command */
	l = mock_terminal__send(cmd, cmd_length);
	CU_ASSERT_EQUAL( l,   cmd_length );

	/* recv challenge */
	l = mock_terminal__recv(recv_buff, recv_max);
	CU_ASSERT_EQUAL( l,   1 + cmd[4] + 2 );

	/* first byte is ACK */
	CU_ASSERT_EQUAL( recv_buff[0], 0x84 );
	/* check status word */
	CU_ASSERT_EQUAL( mock_terminal__last_sw(), SW__OK );


	cmd[4] = 0xFF;

	/* request a challenge of 255 bytes */
	l = mock_terminal__send(cmd, cmd_length);
	CU_ASSERT_EQUAL( l,    cmd_length );

	l = mock_terminal__recv(recv_buff, recv_max);
	CU_ASSERT_EQUAL( l,    1 + cmd[4] + 2 );
	/* first byte is ACK */
	CU_ASSERT_EQUAL( recv_buff[0], 0x84 );
	/* check status word */
	CU_ASSERT_EQUAL( mock_terminal__last_sw(), SW__OK );
}
