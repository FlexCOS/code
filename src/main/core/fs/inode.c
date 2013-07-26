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
#include <io/dev.h>
#include <mm/pool.h>

#include "pools.h"
#include "smartfs.h"


/* ===== Local Functions ==================================================== */
PRIVATE err_t
inode_init_always(Super *s, Inode *inode)
{
	static const struct file_does  empty_f_do = {0};
	static const struct inode_does empty_i_do = {0};

	inode->i_super = s;
	inode->i_mdev = s->s_mdev;
	inode->i_do  = &empty_i_do;
	inode->i_fdo = &empty_f_do;
	inode->i_flags = 0;
	inode->i_count = 1;

	inode->__i_nlink = 1;

	inode->i_state ^= I_NEW;

	return E_GOOD;
}

/**
 *  Allocate a new Inode object. Use either generic version or a call file
 *  system specific implementation. After all initialize inode attributes to
 *  default values.
 */
PRIVATE Inode *
inode_alloc(Super *s)
{
	Inode *inew;

	if (s->s_do->alloc_inode)
		inew = s->s_do->alloc_inode(s);
	else
		inew = ipool_get();

	if (!inew)
		return NULL;

	if (inode_init_always(s, inew)) {
		if (s->s_do->destroy_inode)
			s->s_do->destroy_inode(inew);
		else
			ipool_put(inew);

		return NULL;
	}

	return inew;
}

PRIVATE inline u8
drop_count(Inode *i)
{
	return --(i->i_count);
}

PRIVATE inline void
inc_count(Inode *i)
{
	++i->i_count;
}

PRIVATE inline u8
drop_nlink(Inode *i)
{
	return --(i->__i_nlink);
}

PRIVATE inline u8
inc_nlink(Inode *i)
{
	return ++(i->__i_nlink);
}

/**
 * There is no inode cache right now.
 */
PUBLIC Inode *
inode_find(Super *super, u32 ino)
{
	return NULL;
}



/**
 *  Delete an inode from disk.
 *
 *  This method gets called, if the last inode reference has been put and there
 *  is no reference to the inode in the file system.
 */
PRIVATE void
evict(Inode *inode)
{
	const struct super_does *super_do = inode->i_super->s_do;

	if (super_do->evict_inode)
		super_do->evict_inode(inode);
	else
		generic_delete_inode(inode);

	destroy_inode(inode);
}

/**
 *  Release resources of an inode structure.
 *
 *  Call a specific filesystem logic if it is set or the default one.
 */
PUBLIC void
destroy_inode(Inode *i)
{
	const struct super_does *super_do = i->i_super->s_do;

	if (super_do->destroy_inode)
		super_do->destroy_inode(i);
	else
		ipool_put(i);
}


/**
 * Drop an inode usage.
 *
 * If it becomes unused, call the file system drop method. If it wants the inode
 * to be deleted from disk, just do so. Alternatively we only delete the inode
 * structure itself. There is no caching of unused inodes, yet.
 */
PUBLIC void
iput(Inode *i)
{
	const struct super_does *super_do;
	bool drop;

	if (!i || drop_count(i)) return;

	super_do = i->i_super->s_do;
	if (super_do->drop_inode)
		drop = super_do->drop_inode(i);
	else
		drop = generic_drop_inode(i);

	if (drop)
		evict(i);
	else
		destroy_inode(i);
}

/**
 *  Lookup for an inode with specified inode number to this Super object.
 *
 *  If the inode is not cached, allocate a new one with minimal initialization.
 *  The inode is finally in I_NEW state. The caller has
 *
 *  @return Inode or NULL, if there was any problem (like allocation?)
 */
PUBLIC Inode *
iget(Super *super, u32 ino)
{
	Inode *inode;

	inode = inode_find(super, ino);

	if (inode) {
		inc_count(inode);
		return inode;
	}

	inode = inode_alloc(super);

	if (inode) {
		inode->i_state = I_NEW;
		inode->i_ino   = ino;
		// XXX put it into a cache
	}

	return inode;
}



PUBLIC err_t
inode_pull(Inode *inode)
{
	err_t err;
	const struct super_does *super_do = inode->i_super->s_do;
	const struct inode_does *inode_do = inode->i_do;

	if (inode_do->read)
		err = inode_do->read(inode);
	else
		err = super_do->read_inode(inode);

	if (err) return err;

	/* well we have pulled basic file information. now lets do some ugly
	 * stuff and allocate an array and read complete 'section map'
	 * For each section there should exist one section_map upon file
	 * creation that holds section utilization and the data location. */
	inode->i_smap = malloc(inode_smap_size(inode));

	if (!inode->i_smap)
		return E_NOMEM;

	err = mdev_read(inode->i_mdev,
	                inode->i_smap_loc,
		        inode_smap_size(inode),
		        DEST(inode->i_smap));

	return err;
}

