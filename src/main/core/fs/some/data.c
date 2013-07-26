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

#include <string.h>

#include "somefs.h"
#include "data.h"

PUBLIC void
some_sb_clean( SomeSB *sb )
{
	memset(DEST(sb), 0x00, sizeof(*sb));
	sb->magic = SOMEFS_MAGIC;
}

PUBLIC void
some_df_clean( SomeDF *df )
{
	u32 i;
	FOR_EACH_DF_CHILD (i) {
		df->child[i] = UNSET_CHILD;
	}
}

PUBLIC void
some_fh_clean( SomeFH *fh )
{
	memset( DEST(fh), 0x00, sizeof(*fh) );
}

laddr_t
some_df_get_child(const SomeDF *c, fid_t f)
{
	int i;
	if (f == 0) return 0;

	FOR_EACH_DF_CHILD (i) {
		if (c->child[i].fid == f)
			return c->child[i].addr;
	}

	return 0;
}


/** write a mapping (fid->addr) to the directory dir
 *  @return E_GOOD on success
 */
err_t
some_df_put_child(SomeDF *dir, laddr_t addr, fid_t fid)
{
	u16 i, pos;

	CHECK_PARAM__NOT_ZERO(addr);
	CHECK_PARAM__NOT_ZERO(fid);
	CHECK_PARAM__NOT_NULL(dir);

	pos = 0;
	FOR_EACH_DF_CHILD (i) {
		/* -> do not support hardlinks
		 * -> FID must be unique */
		if ( dir->child[i].fid  == fid ||
		     dir->child[i].addr == addr )
		{
			return E_FS_EXIST;
		}
		/* save the first unused child slot */
		if (pos == 0 && dir->child[i].fid == 0)
			pos = i + 1;
	}

	if (pos) {
		dir->child[pos-1].fid = fid;
		dir->child[pos-1].addr = addr;
		return E_GOOD;
	} else {
		return E_FS;
	}
}
