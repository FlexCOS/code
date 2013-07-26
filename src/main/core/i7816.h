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

enum I7_FileDescriptor {
	MASK_ACCESS       = 0x80,
	MASK_TYPE         = 0x38,
	MASK_STRUCTURE    = 0x07
};

enum I7_FileAccess {
	NOT_SHARED   = 0x00,
	SHAREABLE    = 0x80
};
/* NOTE DF matching is not precise, since MASK_TYPE omits bits 1 to 3. */
enum I7_FileType {
	DF           = 0x38,
	EF_WORKING   = 0x00,
	EF_INTERN    = 0x08,
	EF           = EF_WORKING
};
enum I7_FileStructure {
	TRANSPARENT    = 0x01,
	LINEAR_FIX     = 0x02,
	LINEAR_FIX_TLV = 0x03,
	LINEAR_DYN     = 0x04,
	LINEAR_DYN_TLV = 0x05,
	CYCLIC_FIX     = 0x06,
	CYCLIC_FIX_TLV = 0x07
};

enum I7_LifeCycleStatus {
	LCS_NOT_SET    = 0x00,
	CREATION       = 0x01,
	INIT           = 0x03,
	OP_ACTIVED     = 0x05,
	OP_DEACTIVATED = 0x04,
	TERMINATION    = 0x0C
};

/* Tags for application context have bits 8 and 7 set to '01'. They are some
 * kind of top level data objects. In terms of BER-TLV their content might be
 * primitve or constructed. */
enum I7_APP_TAGS {
	TAG_FMD      = 0x64, /* File Management Data */
	TAG_FCP      = 0x62, /* File Control Parameter */
	TAG_FCI      = 0x6F, /* FCI Template - a set of FMD and FCP objects */
	TAG_OFFSET   = 0x54, /* Data Offset */
	TAG_DATA     = 0x53, /* Discretionary Data Object */
	TAG_FILE_REF = 0x51  /* File Reference Data */
};

enum I7_FCP_Tags {
	TAG_80 = 0x80, /* number of data bytes in a file */
	TAG_81 = 0x81, /* number of data bytes in a file, incl. struct information */
	TAG_82 = 0x82, /* file descriptor byte plus optional fields */
	TAG_83 = 0x83, /* File Identifier */
	TAG_84 = 0x84, /* DF name */
	TAG_85 = 0x85, /* Proprietary information */
	TAG_8A = 0x8A, /* Lyfe cycle status byte */
	TAG_88 = 0x88, /* Short EF identifier */

	TAG_86 = 0x86, /* Sec. attr in proprietary format */
	TAG_8B = 0x8B, /* Sec. attr referencing expanded format */
	TAG_8C = 0x8C, /* Sec. attr in compact format */
	TAG_A0 = 0xA0, /* Sec. attr template for data objects */
	TAG_A1 = 0xA1, /* Sec. attr template in proprietary format */

	TAG_87 = 0x87, /* Identifier of an EF containing an extension of FCI */
	TAG_8D = 0x8D, /* Identifier of an EF containing sec. env. templates */

	TAG_8E = 0x8E, /* Channel security attribute */
	/* ... */

	FCP_LCS      = TAG_8A,
	FCP_SHORT_EF = TAG_88,
	FCP_FID      = TAG_83,
	FCP_DF_NAME  = TAG_84,
	FCP_FD_PLUS  = TAG_82
};

/** special file identifier */
enum I7_FID_Reserved {
	R1    = 0x3FFF,     /**< reserved */
	MF    = 0x3F00,     /**< master file  */
	EFDIR = 0x2F00,     /**< absolute path: MF/EFDIR is reserved */
	EFATR = 0x2F01,     /**< EF.ATR as MF as parent */
	ROOT  = MF,         /**< a root alias */
	EOP   = 0x0000      /**< (reserved) used to terminate a path */
};

/*  short identifier range is from 1 to 30 with a left shift of 3. Bits 1 to 3
 *  are set to zero, e.g., short identifier 1 is in binary written as 00001000 */
enum I7_SFID_Reserved {
	CURRENT = 0x00,      /**< reference current EF */
	EF_DIR  = 0xF8       /**< at MF level this references EF.DIR */
};
/**
 * explicit, implicit, none -- see 7816-4 section 5.3.3.1
 */
enum I7_SFID_Type {
	SFID_IMPLICIT = 0x00,
	SFID_NONE     = 0x01,
	SFID_EXPLICIT = 0x02
};

static inline bool
i7_shareable_file(u8 fdb)
{
	return (fdb & MASK_ACCESS) > 0;
}

static inline enum I7_FileType
i7_ftype(u8 fdb)
{
	return fdb & MASK_TYPE;
}

static inline enum I7_FileStructure
i7_stype(u8 fdb)
{
	return fdb & MASK_STRUCTURE;
}
