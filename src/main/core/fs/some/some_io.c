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

/* System includes */
#include <flxlib.h>
#include <i7816.h>
/* Dependency includes */
#include <io/dev.h>

/* Package includes */
#include "somefs.h"
#include "data.h"
#include "some_io.h"

/* ===== Prototypes ========================================================= */
PRIVATE bool sb_is_valid( SomeSB *, MemDev * );
PRIVATE bool fh_is_valid( SomeFH * );
PRIVATE inline bool sb_is_invalid( SomeSB *, MemDev * );
PRIVATE inline bool fh_is_invalid( SomeFH * );

/* ===== Local functions =================================================== */
PRIVATE bool
sb_is_valid( SomeSB *sb, MemDev *mdev )
{
	if (sb->magic          == SOMEFS_MAGIC &&
	    sb->mf_header      >= sizeof(*sb)  &&
	    sb->next_free_addr >= sizeof(*sb)  &&
	    sb->total_bytes    != 0            &&
	    sb->total_bytes    == mdev->size)
	{
		return true;
	}

	return false;
}

PRIVATE bool
fh_is_valid( SomeFH *fh )
{
#       warning SomeFH validation is temporarly disabled!
	return true;
}

PRIVATE inline bool
sb_is_invalid( SomeSB *sb, MemDev *mdev ) {
	return !sb_is_valid(sb, mdev);
}

PRIVATE inline bool
fh_is_invalid( SomeFH *fh ) {
	return !fh_is_valid(fh);
}

/* ===== Public functions =================================================== */
/**
 * Allocate memory for new file system objects
 */
PUBLIC laddr_t
somefs_alloc(MemDev *mdev, SomeSB *sb, size_t bytes) {
	laddr_t addr;
	err_t   err;

	err = some_sb_read(mdev, sb);
	if (err) return NO_ADDR;

	addr = sb->next_free_addr;

	/* not enough memory left */
	if ((addr + bytes) > sb->total_bytes)
		return NO_ADDR;

	sb->next_free_addr += bytes;

	err = some_sb_write(mdev, sb);
	if (err) return NO_ADDR;

	return addr;
}


PUBLIC err_t
some_fh_read(MemDev *mdev, laddr_t addr, SomeFH *fh)
{
	err_t err;

	err = mdev->read(addr, sizeof(*fh), DEST(fh));
	if (err) return E_HWR;

	if (fh_is_invalid(fh)) {
		some_fh_clean(fh);
		return E_FS;
	}

	return E_GOOD;
}

PUBLIC err_t
some_fh_write(MemDev *mdev, laddr_t addr, SomeFH *fh)
{
	err_t err;

	CHECK_PARAM__NOT_NULL( mdev );
	CHECK_PARAM__NOT_ZERO( addr );
	CHECK_PARAM__NOT_NULL( fh );

	if (fh_is_invalid(fh)) return E_BAD_PARAM;

	err = mdev->write(addr, sizeof(*fh), SRC(fh));
	if (err) return E_HWW;

	return E_GOOD;
}

PUBLIC err_t
some_df_read(MemDev *mdev, laddr_t addr, SomeDF *df)
{
	err_t err;
	some_df_clean(df);

	err = mdev->read(addr, sizeof(*df), DEST(df));
	if (err) return E_HWR;

	return E_GOOD;
}


PUBLIC err_t
some_df_write(MemDev *mdev, laddr_t addr, SomeDF *df)
{
	err_t err;

	err = mdev->write(addr, sizeof(*df), SRC(df));
	if (err) return E_HWW;

	return E_GOOD;
}

PUBLIC err_t
some_sb_read(MemDev *mdev, SomeSB *sb)
{
	err_t err;

	CHECK_PARAM__NOT_NULL(mdev);
	CHECK_PARAM__NOT_NULL(sb);

	some_sb_clean(sb);
	err = mdev->read(0, sizeof(*sb), DEST(sb));
	if (err) return E_INTERN | err;

	if (sb_is_invalid(sb, mdev)) {
		some_sb_clean(sb);
		return E_FS;
	}

	return E_GOOD;
}

PUBLIC err_t
some_sb_write(MemDev *mdev, SomeSB *sb)
{
	err_t err;

	CHECK_PARAM__NOT_NULL (sb);
	CHECK_PARAM__NOT_NULL (mdev);

	if (sb_is_invalid(sb, mdev))
		return E_BAD_PARAM;

	err = mdev->write(0, sizeof(*sb), SRC(sb));
	if (err) return E_INTERN | err;

	return E_GOOD;
}

PUBLIC err_t
init_some_superblock(MemDev *mdev, SomeSB *sb)
{
	err_t err;
	err = mdev->read(0, sizeof(*sb), DEST(sb));

	if (err) return E_INTERN | err;

	return E_GOOD;
}
