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
#include <mm/pool.h>
#include <fs/pools.h>
#include <fs/smartfs.h>

/**
 * Allocate a file object from file array buffer.
 *
 * @return NULL if there is no free object available.
 */
PRIVATE inline File *
file_alloc()
{
	return fpool_get();
}
/**
 * Put a file object back into its array buffer.
 *
 * This operations marks this pointer as unused and sets input file structure
 * to zero.
 */
PRIVATE inline err_t
file_free(File *f)
{
	return fpool_put(f);
}

PUBLIC err_t
fput(File *f)
{
	return file_free(f);
}

PUBLIC File *
fget()
{
	return file_alloc();
}

File * fd_lookup(u32 id) { return fpool_lookup(id); }
u32    fd_get(File *f)   { return fpool_id(f); }

/** Default shortcut: redirect to super operations */
PUBLIC err_t
simple_file_do_open(Inode *i, File *f )
{
	CHECK_PARAM__NOT_NULL(i);
	CHECK_PARAM__NOT_NULL(f);

	if (f->f_dentry == NULL) return E_NULL_PTR;

	// XXX count file handles on inode?!
	f->f_do    = i->i_fdo;
	f->pos     = 0;
	f->section = 0;

	return E_GOOD;
}

/**
 *
 */
PUBLIC err_t
simple_file_do_release(Inode *i, File *f)
{
	if (f->f_dentry->d_inode != i) return E_BAD_PARAM;

	dput(f->f_dentry);

	// release dentry object
	f->f_dentry = NULL;
	f->f_do = NULL;

	return E_GOOD;
}

