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
#include <i7816.h>
#include <array.h>

#include <io/dev.h>
#include <fs/smartfs.h>

#include "somefs.h"
#include "smartfs_impl.h"
#include "data.h"
#include "some_io.h"

PRIVATE const laddr_t SUPER_BLOCK_START = 0;

PUBLIC extern const struct super_does somefs_super_does;

/**
 *  File system creation is done by writing a Superblock and Root directory at
 *  the beginning of some memory device.
 */
PUBLIC err_t
somefs_mkfs(MemDev *mdev)
{
	err_t  err;
	SomeDF df;
	SomeFH fh;
	SomeSB sb;
	u8     __fsdata[sizeof(SomeDF) + sizeof(SomeSB) + sizeof(SomeFH)];
	Array  fsdata = CArray(__fsdata);

	/* Step 01: initialize */
	some_sb_clean(&sb);
	some_df_clean(&df);
	some_fh_clean(&fh);
	/* calculate address references */
	sb.mf_header = SUPER_BLOCK_START + sizeof(sb);
	fh.data = sb.mf_header + sizeof(fh);
	fh.sections = 1;
	fh.sec_size = sizeof(df);
	fh.fdb = DF;
	sb.next_free_addr = fh.data + sizeof(df);
	sb.total_bytes = mdev->size;

	/* write structures to memory device */
	array_append(&fsdata, SRC(&sb), sizeof(sb));
	array_append(&fsdata, SRC(&fh), sizeof(fh));
	array_append(&fsdata, SRC(&df), sizeof(df));

	if (mdev->size < fsdata.length) return E_FS;

	if (array_bytes_left(&fsdata)) return E_INTERN;

	err = mdev->write(0, fsdata.length, SRC(fsdata.v));
	if (err) return E_INTERN | err;

	return E_GOOD;
}

PUBLIC err_t
somefs_mount(MemDev *mdev, Mount *mnt)
{
	err_t  err;
	Super  *s;
	Inode  *i;
	SomeSB *sb;

	CHECK_PARAM__NOT_NULL (mdev);
	CHECK_PARAM__NOT_NULL (mnt);

	s = mnt->super;

	/*  */
	sb = malloc(sizeof(*sb));
	if (sb == NULL) return E_ALLOC;

	err = some_sb_read(mdev, sb);
	if (err) return E_INTERN | err;

	/* reading super block was successful
	 * write file system context to mount point */
	s->s_mdev = mdev;
	s->s_ctx  = sb;
	s->s_do   = &somefs_super_does;

	i = iget(s, sb->mf_header);
	if (i == NULL) return E_INTERN;

	if (somefs_do_read_inode(i)) {
		destroy_inode(i);
		return E_FS;
	}

	dentry_attach(mnt->droot, i);

	return E_GOOD;
}

