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

struct __packed apdu_header {
	u8	CLA;
	u8	INS;
	u8	P1;
	u8	P2;
};

struct command_apdu {
	union {
		u8  *const msg;
		const struct apdu_header *const header;
	};
	union {
		u8  *const data;
		u8  *__data;
	};

	u16 length;
	union {
		const u16 Lc;
		u16 __Lc;
	};
	union {
		const u16 Le;
		u16 __Le;
	};
};

typedef struct command_apdu CmdAPDU;
/* return status word type */
typedef sw_t (*fp_handle_cmd_apdu)(const CmdAPDU *);

PUBLIC fp_handle_cmd_apdu apdu_get_cmd_handler(const CmdAPDU *);

PUBLIC err_t apdu_validate_cmd(CmdAPDU *);


#define SW__expect_LE(xx) ( SW__WRONG_LE | (xx & 0xff) )

enum Status_Word {
	SW__OK                       = 0x9000,
	SW__FUNCTION_NOT_SUPPORTED   = 0x6A81,
	SW__INCORRECT_P1_P2          = 0x6A86,
	SW__NOT_IMPLEMENTED          = 0x6AFF,
	SW__WRONG_LENGTH             = 0x6700,
	SW__WRONG_LE                 = 0x6c00,
	SW__WRONG_DATA               = 0x6A80,
	/* Warnings - non-volatile memory is unchanged */
	SW__EOF                      = 0x6282,
	SW__FILE_DEACTIVATED         = 0x6283,
	SW__WRONG_FCI                = 0x6284,
	SW__FILE_TERMINATED          = 0x6285,
	SW__FILE_NOT_FOUND           = 0x6A82,
	SW__FILE_EXISTS              = 0x6A89,
	SW__FILE_FILLED              = 0x6381,
	SW__LC_TLV_CONFLICT          = 0x6A85,
	SW__NOT_ALLOWED              = 0x6986,
	SW__NO_EF                    = SW__NOT_ALLOWED,
	SW__RECORD_NOT_FOUND         = 0x6A83,

	// legacy sosse stuff
	SW_ACCESS_DENIED=0x6982,
	SW_AUTH_BLOCKED=0x6983,
	SW_AVAILABLE=0x6100,
	SW_CHANGE_MEMORY_ERROR=0x6581,	// Unused
	SW_COUNTER=0x63C0,
	SW_FILE_TO_SHORT=0x6A84,
	SW_INCOMPATIBLE_FILE=0x6981,
	SW_LC_INCONSISTEND_WITH_P1P2=0x6A87,	// Unused
	SW_NOT_ALLOWED=0x6986,
	SW_NOT_SUPPORTED=0x6A81,	// Unused
	SW_OK=0x9000,
	SW_OTHER=0x6F00,
	SW_REF_DATA_INVALID=0x6984,
	SW_REF_DATA_NOT_FOUND=0x6A88,
	SW_VERIFCATION_FAILED=0x6300,	// Unused
	SW_WRONG_CLA=0x6E00,
	SW_WRONG_CONDITION=0x6985,
	SW_WRONG_INS=0x6D00,
	SW_WRONG_LE=0x6C00,
	SW_WRONG_LEN=0x6700,
	SW_WRONG_P1P2=0x6A86,
	SW_WRONG_REFERENCE=0x6B00
};

