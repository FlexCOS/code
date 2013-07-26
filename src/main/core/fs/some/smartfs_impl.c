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

#include <io/dev.h>
#include <mm/pool.h>
#include <fs/smartfs.h>
#include <fs/pools.h>

#include "somefs.h"
#include "data.h"
#include "some_io.h"

#include "smartfs_impl.h"

#define MAX_RECORDS 16

extern laddr_t somefs_alloc(MemDev *, SomeSB *, size_t);



PRIVATE err_t sync_to_header(Inode *, SomeFH *);
PRIVATE err_t sync_to_inode(Inode *, SomeFH *);
PRIVATE laddr_t create_fresh_smap(Super *, u8, u16);


/*===== Global Constants =====================================================*/
PUBLIC const struct super_does somefs_super_does = {
	.alloc_inode = somefs_super_do_alloc_inode,
	.destroy_inode = somefs_super_do_destroy_inode,
	.evict_inode = NULL,
	.drop_inode = NULL,
	.read_inode = somefs_do_read_inode,
	.write_inode = somefs_do_write_inode,
};

PUBLIC const struct inode_does somefs_inode_does = {
	.lookup = somefs_inode_do_lookup,
	.create = somefs_inode_do_create,
	.mkdir = NULL,
	.write = somefs_do_write_inode,
	.read = somefs_do_read_inode
};

PRIVATE const struct file_does somefs_file_does = {
	.open = NULL,
	.release = NULL,
	.read = somefs_file_do_read,
	.write = somefs_file_do_write,
	.seek = NULL
};

/**
 *  This array is used to read meta data about real record utilization.
 *
 *  FIXME Documentation is obsolete
 *  Every data field is prepended by a sequence of multiple two byte values.
 *  There are two bytes for each record indicating record load and one last two
 *  byte value giving the number of total bytes each record has been allocated.
 */
//  PRIVATE u16 record_size[MAX_RECORDS + 1];


static inline laddr_t addr_of_ino( u32 ino ) { return ino; }
static inline u32     addr_to_ino( laddr_t a ) { return a; }


/* ----- Super Implementations ---------------------------------------------- */
PUBLIC Inode *
somefs_super_do_alloc_inode(Super *s)
{
	SomeFH *fh;
	Inode *inew;

	inew = ipool_get();

	if (!inew) return NULL;

	fh = malloc(sizeof(*fh));

	if (!fh) {
		ipool_put(inew);
		return NULL;
	}

	inew->i_ctx = fh;
	return inew;
}

PUBLIC void
somefs_super_do_destroy_inode(Inode *i)
{
	free(i->i_ctx);
	ipool_put(i);
}

/**
 * Fill an inode object with new data from file system.
 */
PUBLIC err_t
somefs_do_read_inode(Inode *i)
{
	err_t   err;
	laddr_t addr;
	Super   *s  = i->i_super;
	SomeFH  *fh = i->i_ctx;

	addr = addr_of_ino(i->i_ino);
	if (addr == 0) return E_BAD_PARAM | E_FS_INO;

	err = some_fh_read( s->s_mdev, addr, fh );
	if (err) return E_INTERN | err;

	sync_to_inode(i, fh);
	return E_GOOD;
}

/**
 * Synchronize an Inode to non-volatile storage.
 *
 * @Param[in] inode must not be NULL
 */
PUBLIC err_t
somefs_do_write_inode(Inode *i)
{
	err_t   err;
	laddr_t addr;
	Super   *s  = i->i_super;
	SomeFH  *fh = i->i_ctx;

	CHECK_PARAM__NOT_NULL (s);
	CHECK_PARAM__NOT_NULL (fh);

	addr = addr_of_ino(i->i_ino);
	if (addr == 0) return E_BAD_PARAM | E_FS_INO;

	sync_to_header(i, fh);

	err = some_fh_write(s->s_mdev, addr, fh);
	if (err) return E_INTERN | err;

	return E_GOOD;
}

/* ----- Inode Operation Interface ------------------------------------------ */

PUBLIC err_t
somefs_inode_do_lookup(Inode *iparent, Dentry *d)
{
	SomeDF df;
	Inode  *child_inode;
	Super  *s;
	err_t  err;
	u32    ino;

	CHECK_PARAM__NOT_NULL (iparent);
	CHECK_PARAM__NOT_NULL (d);
	CHECK_PARAM__NOT_ZERO (d->d_name);

	s = iparent->i_super;
	CHECK_PARAM__NOT_NULL (s);
	// XXX too lazy!!

	err = some_df_read( iparent->i_mdev, iparent->i_data, &df );
	if (err) return E_INTERN | err;

	/* read ino from directory */
	ino = some_df_get_child(&df, d->d_name);
	// if ino is 0 <=> no such file or directory
	if (!ino) return E_NOENT;

	child_inode = iget(s, ino);

	err = inode_pull(child_inode);

	/* connect dentry with inode */
	dentry_attach(d, child_inode);

	return E_GOOD;
}

/**
 *  Create a file in directory denoted by iparent
 *
 *  @param[in] iparent, an inode pointing to a directory object
 *  @param[in,out] d a dentry, that has set d_name
 */
PUBLIC err_t
somefs_inode_do_create(Inode *iparent, Dentry *d, const Attr *attr)
{
	Inode  *inew;
	Super  *s;
	SomeFH *fh;
	SomeDF df;
	u32    addr_h;
	u32    addr_b;
	err_t  err;

	CHECK_PARAM__NOT_NULL(iparent);
	CHECK_PARAM__NOT_NULL(d);
	CHECK_PARAM__NOT_ZERO(d->d_name);

	s = iparent->i_super;
	CHECK_PARAM__NOT_NULL(s);

	/* this will fail if iparent does not point to a directory */
	err = some_df_read( iparent->i_mdev, iparent->i_data, &df);
	if (err) return E_INTERN | err;

	/* Allocate space for file header and data body  */
	addr_h = somefs_alloc(s->s_mdev, s->s_ctx, sizeof(SomeFH));

	addr_b = create_fresh_smap(s, attr->sections, attr->sec_size);
	inew = iget(s, addr_to_ino(addr_h));
	if (!inew) return E_ALLOC;

	/* SomeFH structure has already been allocated and attached to
	 * the inodes private context. */
	fh = inew->i_ctx;

	/* XXX export: prepare as EF */
	fh->fdb = attr->iso7816.fdb;
	fh->lcs = attr->iso7816.lcs;
	fh->data = addr_b;
	fh->sections = attr->sections;
	fh->sec_size = attr->sec_size;

	sync_to_inode(inew, fh);

	if ((err = some_fh_write( inew->i_mdev, inew->i_ino, fh))
	||  (err = some_df_put_child(&df, inew->i_ino, d->d_name))
	||  (err = some_df_write( iparent->i_mdev, iparent->i_data, &df )))
	{
		/* XXX introduce flag orphaned? */
		iput(inew);
		return E_INTERN | err;
	}

	dentry_attach(d, inew);
	return E_GOOD;
}

PUBLIC err_t
somefs_inode_do_mkdir(Inode *iparent, Dentry *d)
{
	return E_NO_LOGIC;
}

/**
 *  Retrieve data length and maximum size of a single section.
 *
 *  FIXME export this
 */
PRIVATE void
somefs_sec_info(const Inode *i, u8 sec, u16 *length, u16 *size)
{
	/* NOTE This is for fixed length files only */
	if (sec > i->i_sections) {
		*length = 0;
		*size   = 0;
	}

	*size = i->i_size / i->i_sections;
	*length = i->i_smap[sec].length;
}

PRIVATE void
somefs_sec_update(const Inode *i, u8 sec, u16 length)
{
	/* nothing to do if length has not been increased */
	if (i->i_smap[sec].length >= length) return;

	i->i_smap[sec].length = length;

	mdev_write(i->i_mdev,
	           i->i_smap_loc,
	           i->i_sections * sizeof(struct section_map),
		   SRC(i->i_smap));
}

/**
 *
 */
PUBLIC size_t
somefs_file_do_read(File *f, buff8_t dest, size_t bytes)
{
	Inode  *i = f->f_dentry->d_inode;
	MemDev *dev = i->i_mdev;
	u16 rmax, rlen;
	u32 addr;

	if (f->section >= i->i_sections) return EOF;

	somefs_sec_info(i, f->section, &rlen, &rmax);

	if (f->pos >= rlen) return EOF;

	bytes = MIN((rlen - f->pos), bytes);

	addr  = i->i_smap[f->section].addr;
	addr += f->pos;

	if (dev->read(addr, bytes, DEST(dest)))
		return 0;

	f->pos += bytes;
	return bytes;
}


PUBLIC size_t
somefs_file_do_write(File *f, const buff8_t src, size_t bytes)
{
	Inode  *i;
	MemDev *dev;
	laddr_t addr;

	CHECK_PARAM__NOT_NULL (f->f_dentry);
	CHECK_PARAM__NOT_NULL (f->f_dentry->d_inode);

	i   = f->f_dentry->d_inode;
	dev = i->i_mdev;

	/* This version has a static file size */
	if (f->pos > i->i_size) return EOF;

	if (f->section >= i->i_sections) return EOF;

	addr  = i->i_smap[f->section].addr;
	addr += f->pos;

	bytes = MIN ((i->i_size - f->pos), bytes);

	if (dev->write(addr, bytes, SRC(src)))
		return 0;

	f->pos += bytes;
	/* update section length if needed */
	somefs_sec_update(i, f->section, f->pos);
	return bytes;
}


/* ----- Utility methods ---------------------------------------------------- */
PRIVATE laddr_t
create_fresh_smap(Super *s, u8 sections, u16 section_size)
{
	struct section_map smap[8] = {{0}};
	u32 i;
	u32 off;
	u8      sec;
	u32     raw_size;
	laddr_t data_base;
	laddr_t smap_base;

	raw_size  = sections * sizeof(struct section_map);
	raw_size += sections * section_size;

	/* preallocate continous memory for section map and sections itself. */
	smap_base = somefs_alloc(s->s_mdev, s->s_ctx, raw_size);
	data_base = smap_base + (sections * sizeof(struct section_map));

	if (!smap_base)
		return 0;

	/* well, now the ugly part: set each section mapping and 
	 * write it to device */
	for (sec = 0, i = 0;
	     sec < sections;
	     sec += i)
	{
		for (i = 0; i < LENGTH(smap) && (sec + i) < sections; i++) {
			smap[i].addr = data_base;
			data_base += section_size;
		}

		off = sec * sizeof(struct section_map);
		mdev_write(s->s_mdev, smap_base + off, i * sizeof(struct section_map), SRC(smap));
	}

	return smap_base;
}

/* XXX unfortunately not really useful o.O */
PRIVATE err_t
sync_to_header(Inode *i, SomeFH *h)
{
	CHECK_PARAM__NOT_NULL (i);
	CHECK_PARAM__NOT_NULL (h);

	h->lcs = inode_lcs_get(i);

	return E_GOOD;
}

PRIVATE err_t
sync_to_inode(Inode *i, SomeFH *h)
{
	CHECK_PARAM__NOT_NULL (i);
	CHECK_PARAM__NOT_NULL (h);

	i->i_fdo = &somefs_file_does;
	i->i_do = &somefs_inode_does;

	i->i_data  = h->data;
	i->i_smap_loc = h->data;

	i->i_size = h->sections * h->sec_size;
	i->i_fdb = h->fdb;
	i->i_sections = h->sections;

	inode_lcs_set(i, h->lcs);

	return E_GOOD;
}
