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

#pragma once

enum POOLS {
	IPOOL_SIZE = FS_MAX_ACTIVE_INODES,
	DPOOL_SIZE = FS_MAX_ACTIVE_DENTRIES,
	FPOOL_SIZE = FS_MAX_ACTIVE_FILES
};

/* compile time checks */
#ifdef DEBUG
struct __check_pool_size {
	u8 ipool_size[0 - FPOOL_SIZE%8];
	u8 dpool_size[0 - DPOOL_SIZE%8];
	u8 fpool_size[0 - IPOOL_SIZE%8];
}
#endif

struct pool;
struct inode;
struct dentry;

extern struct pool *const ipool;
extern struct pool *const dpool;
extern struct pool *const fpool;


static inline err_t
ipool_reset(void) { return pool_reset(ipool); }

static inline struct inode *
ipool_get(void) { return (struct inode *) pool_get(ipool); }

static inline err_t
ipool_put(struct inode *i) { return pool_put(ipool, i); }

static inline struct inode *
ipool_lookup(u32 id) { return (struct inode *) pool_lookup(ipool, id); }

static inline u32
ipool_id(const struct inode *i) { return pool_id(ipool, i); }




static inline err_t
dpool_reset(void) { return pool_reset(dpool); }

static inline struct dentry *
dpool_get(void) { return (struct dentry *) pool_get(dpool); }

static inline err_t
dpool_put(struct dentry *d) { return pool_put(dpool, d); }

static inline struct dentry *
dpool_lookup(u32 id) { return (struct dentry *) pool_lookup(dpool, id); }

static inline u32
dpool_id(const struct dentry *d) { return pool_id(dpool, d); }



static inline err_t
fpool_reset(void) { return pool_reset(fpool); }

static inline struct file *
fpool_get(void) { return (struct file *) pool_get(fpool); }

static inline err_t
fpool_put(struct file *f) { return pool_put(fpool, f); }

static inline struct file *
fpool_lookup(u32 id) { return (struct file *) pool_lookup(fpool, id); }

static inline u32
fpool_id(const struct file *f) { return pool_id(fpool, f); }

static inline err_t
pools_reset(void)
{
	return ipool_reset()
             | dpool_reset()
	     | fpool_reset();
}
