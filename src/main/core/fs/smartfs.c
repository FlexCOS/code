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
#include <channel.h>
#include <string.h>

#include <mm/pool.h>
#include <io/dev.h>

#include "path.h"
#include "pools.h"
#include "smartfs.h"

PRIVATE Super  single_super;
PUBLIC struct fs_mount mnt = {0};

PRIVATE Dentry *
dentry_alloc(Super *super, fid_t name) {
	Dentry *dentry = dpool_get();

	if (dentry) {
		dentry->d_name  = name;
		dentry->d_inode = NULL;
		dentry->d_count = 1;
		dentry->d_sb    = super;
	}

	return dentry;
}

PRIVATE void
dentry_free(Dentry *dentry)
{
	dpool_put(dentry);
	return;
}

/**
 * Release the dentries inode.
 */
PRIVATE void
dentry_iput(Dentry *d)
{
	Inode *inode = dentry_detach(d);
	if (inode) {
		d->d_inode = NULL;
		/* drop inode usage in a common way */
		iput(inode);
	}
}

/**
 * NOTE! We expect the caller to increment usage count
 */
PUBLIC void
dentry_attach( Dentry *d, Inode *i )
{
	d->d_inode = i;
	d->d_sb = i->i_super;
}

/**
 *
 */
PUBLIC Inode *
dentry_detach(Dentry *d)
{
	Inode *i = d->d_inode;

	d->d_inode = NULL;
	d->d_sb = NULL;

	return i;
}

PUBLIC err_t
smartfs_mount_root( MemDev *mdev, fp_do_mount do_mount )
{
	err_t err;

	/* there is no allocator for struct super objects. */
	mnt.super = &single_super;
	mnt.droot = dentry_alloc(mnt.super, MF);
	// XXX clean objects

	err = do_mount(mdev, &mnt);
	if (err)
		return E_INTERN | err;

	current->df = mnt.droot;

	return E_GOOD;
}



PRIVATE void
dentry_release(Dentry *d)
{
	// XXX remove dentry from cache, or introduce a flag that forces us to
	// lookup the dentry next time

	dentry_iput(d);

	// FIXME the cache should release resources
	dentry_free(d);

	return;
}

PUBLIC void
dput(Dentry *dentry)
{
	if (dentry) {
		dentry->d_count--;
		if (!dentry->d_count)
			dentry_release(dentry);
	}
}

PRIVATE Dentry *
lookup_cache(fid_t fid, Dentry *dir, bool *need_lookup)
{
	Dentry *dentry;

	*need_lookup = true;

	dentry = dentry_alloc(dir->d_sb, fid);

	return dentry;
}

PRIVATE err_t
lookup_real(Inode *dir, Dentry *dentry)
{
	err_t err;

	err = dir->i_do->lookup(dir, dentry);

	if (err) {
		dentry_free(dentry);
	}

	return err;
}

/**
 *  Step into lookup logic of a single file system object in a known directory.
 *
 *  There is a two way lookup. First we try to locate the object in a cache
 *  structure. If the cache does not contain such an element, it creates one and
 *  we are advised to do a real lookup from file system implementation.
 *  (i.e. a lookup from storage)
 *
 *  There is no backreference to parent objects yet ...
 */
PUBLIC Dentry *
dentry_lookup(Dentry *dir, fid_t name)
{
	Dentry *dentry;
	bool   need_lookup;
	err_t  err;

	need_lookup = false;

	dentry = lookup_cache(name, dir, &need_lookup);
	if (!dentry) {
		return NULL;
	}

	if (need_lookup) {
		err = lookup_real(dir->d_inode, dentry);
		if (err) {
			return NULL;
		}
		/* At this point dentry may not be complete. There was just a
		 * successful inode lookup. The inode objects needs to be read.
		 */
		inode_pull(dentry->d_inode);
	}

	return dentry;
}

/**
 *  Create a new file system object under given Dentry.
 *
 *  @return Dentry to new object or NULL on any failure.
 */
PUBLIC Dentry *
dentry_create(Dentry *dir, fid_t name, Attr *attr)
{
	Inode  *parent = dir->d_inode;
	Dentry *new;
	err_t  err;

	new = dentry_alloc(parent->i_super, name);

	err = parent->i_do->create(parent, new, attr);
	/* ...oh the beauty of quick and dirty workarounds */
	inode_pull(new->d_inode);
	if (err) {
		dentry_free(new);
		new = NULL;
	}
	// XXX insert into cache
	return new;
}


/**
 * This version of lookup allocates a new dentry object on each call. There
 * is no cached data. The caller must release the object.
 *
 * @param[in] path a zero terminated fid_t array
 * @param[out] the dentry object accoding to input path
 */
PUBLIC err_t
smartfs_path_lookup(const path_t path, Dentry **dentry)
{
	Dentry *dir, *next;
	/* temporary relative path */
	path_t tmp_path = path;
	/* curret file identifier points to
	 * next relative file system object */
	fid_t  next_fid;

	*dentry = NULL;

	/* remove leading MF fid */
	if (path_is_absolute(path)) {
		next = mnt.droot;
		tmp_path = ptail(path);
	} else {
		next = current->df;
	}

	next->d_count++;

	while ((next_fid = pwalk(&tmp_path))) {
		dir = next;
		next = dentry_lookup(dir, next_fid);

		if (!next)
			return E_NOENT;

		dput(dir);
	}

	*dentry = next;

	return E_GOOD;
}



PUBLIC err_t
smartfs_reset()
{
	ipool_reset();
	dpool_reset();
	fpool_reset();

	return E_GOOD;
}
