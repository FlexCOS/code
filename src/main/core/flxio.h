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

typedef u32 FILE;


/**
 *  Structural representation of ISO 7816-4 File Control Parameters.
 */
struct i7_fcp {
	u8                fdb;
	u8                lcs;
	u8                dcb;
	u16               fid;
	u8                sfid      : 5;
	u8                sfid_type : 3;
	char              df_name[16];
	union {
		u16       size;
		u16       rsize;
	};
	u16               rcount;
	void              *sec_attr;
};

enum Seek_Whence {
	SEEK_SET = 0x00,
	SEEK_CUR = 0x01,
	SEEK_END = 0x02
};

FILE * f_open(const path_t);

FILE * k_open(const path_t);

FILE * f_info(const path_t, struct i7_fcp *);

FILE * f_create(struct i7_fcp *);

err_t f_close(FILE *);

err_t f_remove(const path_t);

size_t f_write(const void *, size_t, size_t, FILE *);
/**
 *  Seek position within current record.
 */
err_t f_seek(FILE *, s32, enum Seek_Whence);
/**
 *  Seek to start of a specific section within file.
 */
err_t f_seeks(FILE *, s16, enum Seek_Whence);
/**
 *  Tell active section ID
 */
u8    f_tells(FILE *);
/**
 *  Tell byte offset within active record
 */
u16   f_tell(FILE *);
/**
 *  Generic read for any file type.
 */
size_t f_read(void *, size_t, size_t, FILE *);

err_t ch_df_by_path(const path_t);

err_t ch_df_by_name(char[]);

err_t mk_df(const path_t);

