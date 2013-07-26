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

#define FOR_EACH_DF_CHILD(i) for( i = 0; i < SOMEFS_MAX_DIR_ENTRIES; i++ )

struct some_superblock;
struct some_header;
struct some_df;
struct some_df_child;

typedef struct some_superblock SomeSB;
typedef struct some_header     SomeFH;
typedef struct some_df         SomeDF;

#define SOME_FH(type, args...) { .ftype = type, ##args}

PUBLIC void some_df_clean( SomeDF * );
PUBLIC void some_fh_clean( SomeFH * );
PUBLIC void some_sb_clean( SomeSB * );

PUBLIC laddr_t some_df_get_child( const SomeDF *, fid_t );
PUBLIC err_t   some_df_put_child( SomeDF *, laddr_t, fid_t );



struct __packed some_superblock {
	/* a magic number just for validation check */
	u16     magic;
	u32     total_bytes;
	laddr_t mf_header;
	/* allocation data */
	laddr_t next_free_addr;
	u16 flags;
};

struct __packed some_header {
	/* a magic number just for consistency check */
	union {
		const u8 magic;
		u8     __magic;
	};
	/** File Descriptor byte according to ISO 7816 definition */
	u8      fdb;
	/** Life Cycle Status byte */
	u8      lcs;
	/** logical address of data body */
	laddr_t data;
	/** size of the data body */
	u8      sections;
	/** number of bytes that are already allocated for each section */
	u16     sec_size;
};

struct some_df_child {
	fid_t   fid;
	laddr_t addr;
};

/*! Structure of an dedicated file written to memory.
 *
 */
struct some_df {
	struct some_df_child child[SOMEFS_MAX_DIR_ENTRIES];
};

/*!
 *
 */
static const struct some_df_child UNSET_CHILD = {
	.fid = 0,
	.addr = 0
};
