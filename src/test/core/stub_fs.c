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

/* Core library includes. */
#include <flxlib.h>
#include <string.h>

/* API includes. */
#include <io/dev.h>
#include <fs/smartfs.h>

/* file system implementation */
#include <fs/some/somefs.h>

/* hardware device */
#include "stub_memdev.h"

#include "stub_fs.h"


PRIVATE MemDev mdev;

PUBLIC err_t
stub_fs_init()
{
	if (stub_memdev_init(&mdev)
	||  somefs_mkfs(&mdev)
	||  smartfs_mount_root(&mdev, somefs_mount))
	{
		return E_FAILED;
	}

	return E_GOOD;
}

PUBLIC err_t
stub_fs_free()
{
	stub_memdev_release(&mdev);
	smartfs_reset();

	return E_GOOD;
}
