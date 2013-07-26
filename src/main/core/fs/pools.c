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

#include <const.h>
#include <types.h>

#include <mm/pool.h>

#include "smartfs.h"
#include "pools.h"

PRIVATE Inode  iarr[IPOOL_SIZE] = {{0}};
PRIVATE Dentry darr[DPOOL_SIZE] = {{0}};
PRIVATE File   farr[FPOOL_SIZE] = {{0}};

PRIVATE u8     iarr_usage_mask[IPOOL_SIZE/8] = {0};
PRIVATE u8     darr_usage_mask[DPOOL_SIZE/8] = {0};
PRIVATE u8     farr_usage_mask[FPOOL_SIZE/8] = {0};

/* FIXME rename: this is not a cache */
PRIVATE struct pool __ipool = {
		.n     = LENGTH(iarr),
		.msize = sizeof(Inode),
		.memb  = iarr,
		.usage = iarr_usage_mask
};

PRIVATE struct pool __dpool = {
		.n = LENGTH(darr),
		.msize = sizeof(Dentry),
		.memb  = darr,
		.usage = darr_usage_mask
};

PRIVATE struct pool __fpool = {
		.n = LENGTH(farr),
		.msize = sizeof(File),
		.memb  = farr,
		.usage = farr_usage_mask
};

PUBLIC struct pool *const ipool = &__ipool;
PUBLIC struct pool *const dpool = &__dpool;
PUBLIC struct pool *const fpool = &__fpool;
