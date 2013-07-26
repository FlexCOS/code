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

/**
 * flxio.c
 *
 * Created
 *     Author: Alexander Münn
 */

#include <flxlib.h>
#include <i7816.h>
#include <flxio.h>
#include <channel.h>

#include <fs/path.h>
#include <fs/smartfs.h>


PRIVATE err_t
do_open(Dentry *dentry, File *filp)
{
	Inode *inode = dentry->d_inode;

	// XXX set an open flag?
	CHECK_PARAM__NOT_NULL(inode);

	filp->f_dentry = dentry;
	filp->f_do = inode->i_fdo;

	if (inode->i_fdo->open)
		return inode->i_fdo->open(inode, filp);

	return E_GOOD;
}

static bool
is_fcp_supported(struct i7_fcp *fcp)
{
	u8 fs = i7_stype(fcp->fdb);

	if (i7_shareable_file(fcp->fdb)) return false;

	if (!fs) return false;

	/* Do not support any kind of TLV structure */
	if ((fs & ~0x01)
	&&  (fs &  0x01))
	{
		return false;
	}

	/* XXX too confident? */
	return true;
}


static bool
is_fcp_sound(struct i7_fcp *fcp)
{
	if ((!fcp->size)
	||  (!fcp->fid)
	||  (!fcp->fdb))
		return false;

	switch (i7_stype(fcp->fdb)) {
	case TRANSPARENT:
		if (fcp->rcount) return false;
		break;
	/* Any record oriented structure needs at least one one record. */
	default:
		if (!fcp->rcount) return false;
	}

	return true;
}


/**
 *  Create a new file object within working directory.
 *
 *  On successful creation the new file is directly opened.
 *
 *  @return Pointer to internal file descriptor or NULL on failure.
 */
PUBLIC FILE *
f_create(struct i7_fcp *fcp)
{
	Dentry *new;
	File   *file;
	FILE   *fd;
	err_t  err;
	Attr   attr = {{0}, 0};

	if (i7_ftype(fcp->fdb) != EF) return NULL;

	if (!is_fcp_supported(fcp)) return NULL;
	/* check soundness of function parameter 'fcp' */
	if (!is_fcp_sound(fcp)) return NULL;

	attr.iso7816.fdb = fcp->fdb;
	attr.iso7816.lcs = fcp->lcs;
	attr.iso7816.dcb = fcp->dcb;
	/* every file contains sections. On record oriented files this number
	 * is equivalent to record number. Transparent files have only one
	 * section */
	attr.sections = fcp->rcount ? fcp->rcount : 1;
	attr.sec_size = fcp->size;

	/* check for an existing file with given FID */
	new = dentry_lookup(current->df, fcp->fid);
	if (new) {
		dput(new);
		return NULL;
	}

	new = dentry_create(current->df, fcp->fid, &attr);

	if (!new) return NULL;

	file = fget();
	fd   = malloc(sizeof(FILE));
	*fd   = fd_get(file);

	/* memory allocation error */
	if (!file || !fd) return NULL;

	err = do_open(new, file);

	if (err) {
		free(fd);
		fput(file);
		dput(new);
		return NULL;
	}

	return fd;
}

PUBLIC FILE *
f_open(const path_t p)
{
	err_t  err;
	Dentry *dentry;
	FILE   *fd;
	File   *file;

	if (path_length(p) > 4)
		return NULL;

	err = smartfs_path_lookup(p, &dentry);

	if (err) {
		dput(dentry);
		return NULL;
	}

	/*  FIXME !!!
	 *  This is duplicate code (see: f_create). Provide some common file
	 *  pointer instantiation.  */
	file = fget();
	fd   = malloc(sizeof(FILE));
	*fd   = fd_get(file);

	if (!file || !fd) return NULL;

	err = do_open(dentry, file);
	if (err) {
		free(fd);
		fput(file);
		dput(dentry);
		return NULL;
	}

	return fd;
}

PUBLIC err_t
f_close(FILE *fh)
{
	File  *f = fd_lookup(*fh);
	Inode *i = f->f_dentry->d_inode;
	err_t err;

	if (fh == NULL || f == NULL) return E_BADFD;

	if (f->f_do->release) {
		err = f->f_do->release(i, f);
		if (err)
			return E_FAILED;
	}

	dput(f->f_dentry);
	fput(f);

	/* this was allocated through f_create or f_open */
	free(fh);

	return E_GOOD;
}

/**
 * Read some bytes :)
 *
 * @return size_t number of read bytes, or EOF
 */
PUBLIC size_t
f_read(void *dest, size_t mbytes, size_t nmemb, FILE *fd)
{
	File *file = fd_lookup(*fd);
	size_t rbytes = 0;

	if (file) {
		rbytes = file->f_do->read(file, DEST(dest), nmemb * mbytes);
	}

	return rbytes;
}

/**
 * Write some bytes :)
 *
 * @return size_t number of written bytes, or EOF ...
 */
PUBLIC size_t
f_write(const void *src, size_t mbytes, size_t nmemb, FILE *fd)
{
	File *file;
	size_t written = 0;

	if (!fd) return 0;

	file = fd_lookup(*fd);

	if (file) {
		written = file->f_do->write(file, SRC(src), nmemb * mbytes);
	}

	return written;
}

PUBLIC err_t
f_seek(FILE *fd, s32 offset, enum Seek_Whence whence)
{
	File *file;

	CHECK_PARAM__NOT_NULL(fd);

	file = fd_lookup(*fd);

	if (!file) return E_BADF;

	// FIXME!!
#	warning fseek offset parameter has not been validated!
	file->pos = offset;

	return E_GOOD;
}

PUBLIC err_t
f_seeks(FILE *fd, s16 sjump, enum Seek_Whence whence)
{
	File *file;
	s32  sec;
	u16  max;

	CHECK_PARAM__NOT_NULL(fd);

	file = fd_lookup(*fd);

	if (!file) return E_BADF;

	/* FIXME urgs... m( */
	max = file->f_dentry->d_inode->i_sections;

	switch (whence) {
	case SEEK_CUR:
		sec = file->section + sjump;
		break;
	case SEEK_END:
		sec = max + sjump;
	case SEEK_SET:
		sec = sjump;
		break;
	default:
		return E_BAD_PARAM;
	}
	if (sec < 0 || sec > max)
		return E_RANGE;

	file->section = sec;
	file->pos = 0;

	return E_GOOD;
}
