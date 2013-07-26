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

/*! \file afs.h - An abstract definition of file system
 *
 */
#pragma once
#ifndef _SMARTFS_H_
#define _SMARTFS_H_


/* extern objects */
struct mem_dev;




struct super;
struct dentry;
struct inode;

struct super_does;
struct inode_does;
struct dentry_does;
struct file_does;

typedef struct inode       Inode;
typedef struct dentry      Dentry;
typedef struct super       Super;
typedef struct fs_mount    Mount;
typedef struct file        File;

typedef err_t (*fp_do_mount)(struct mem_dev *, Mount *);

/**
 * Smartfs file system implementation is being accessed through this mount
 * point structure. Initialization is done through smartfs_mount_root function.
 */
extern struct fs_mount mnt;

enum Inode_State {
	I_NEW = 0x10,
	I_DIRTY = 0x20,
};

/**
 * A mounted file system is associated to a dentry.
 *
 * @see in linux/mount.h
 */
struct fs_mount {
	/* the root dentry of this mount point */
	struct dentry      *droot;
	/* file system handler */
	struct super       *super;
};


/**
 *  Super is the major hook to any 'smartfs' implementation.
 *
 *  Core operations are centered around Inodes.
 */
struct super {
	const struct super_does *s_do;
	/* point to implementation specific infos */
	void *s_ctx;
	/* further properties following */
	struct mem_dev   *s_mdev;
};


struct section_map {
	u32     length;
	laddr_t addr;
};

/**
 *  Inodes are generic in-memory representations of filesystem objects.
 */
struct inode {
	u32                i_ino;	/* unique id for an inode */
	u8                 i_fdb;      /* contains flags indicating inode type */
	u8                 i_flags;	/* integrity flags */
	u8                 i_state;     /* life time descriptor */
	u8                 i_count;     /* number of references to this inode */
	union {
		const u8   i_nlink;     /* number of references to this file */
		u8         __i_nlink;
	};
	laddr_t            i_data;
	u32                i_size;	/* size of data part */
	u8                 i_sections;  /* number of sections */
	laddr_t            i_smap_loc;	/* where to find section_map */
	struct section_map *i_smap;     /*  */
	void               *i_ctx;	/* file system specific inode context */
	struct super       *i_super;	/* supe_block this inode belongs to */
	const struct inode_does *i_do;  /* hold explicit inode operations */
	const struct file_does  *i_fdo; /* hold default file operations */
	struct mem_dev     *i_mdev;
};

/**
 * Dentries provide hierarchical relation between inodes.
 */
struct dentry
{
	fid_t          d_name;
	u8             d_count;
	struct inode   *d_inode;
	struct super   *d_sb;
	/* filesystem specific infos */

	const struct dentry_does *d_do;
};

/**
 *  The actual file handle.
 *
 *  This structure is filled on opening a filesystem object. First we need a
 *  reference to the object within hierarchical structure, i.e. a Dentry object,
 *  which provides also an Inode.
 *
 *  More important are supported file operations itself. They are implementation
 *  specific: reading, writing, and probably opening, closing and seeking.
 *
 *  The file handle holds context information about location within a file
 *  object, i.e. a section identifier and an offset aka pos within this section.
 *  This generic fragmentation is made with regard to ISO record handling.
 *  Accessing a record almost equals accessing a section. The significant
 *  difference is in section identification. The file system does not provide
 *  any special section selectors except seeking by section index.
 */
struct file
{
	/** Reference to filesystem object. */
	Dentry   *f_dentry; // XXX or some kind of path := { Super, Dentry } ???
	/** Byte offset relative to current record. */
	u16      pos;
	/** Section identifier points to a dedicated part of a file */
	u8       section;
	// struct path f_path ??
	const struct file_does *f_do;
};

typedef struct file_attributes Attr;

/**
 *  Basic file attributes that are needed to create a file.
 */
struct file_attributes {
	struct {
		u8 fdb;
		u8 lcs;
		u8 dcb;
	} iso7816;
	u16	sec_size;
	u8      sections;
};

struct file_does
{
	/* notify file system that we want to open an Inode */
	err_t  (*open) (Inode *, File *);
	/* notify file system that we want to close a file */
	err_t  (*release)(Inode *, File *);
	/* read size_t bytes from struct file into buff8_t */
	size_t (*read) (File *, buff8_t, size_t);
	size_t (*write)(File *, const buff8_t, size_t);
	/* Seeking through an open file depends on file system capabilities.
	 * The file system is asked seek to position size_t according to the
	 * next IO operation. */
	size_t (*seek) (File *, size_t);
	err_t  (*readdir)(File *, fid_t *);
};

struct super_does {
	/* create and initialize a new inode object */
	Inode *(*alloc_inode)(Super *);
	/* Release resources of struct inode */
	void   (*destroy_inode)(Inode *);
	/* delete given inode from disk */
	err_t  (*evict_inode)(Inode *);
	/* drop inode usage */
	u8     (*drop_inode)(Inode *);

	err_t  (*read_inode)(Inode *);   /* default read operation  */
	err_t  (*write_inode)(Inode *);  /* default write operation */
};

/**
 *  A filesystem interface to operate on inodes.
 */
struct inode_does {
	/* look for an inode */
	err_t (*lookup)(Inode *, Dentry *);
	err_t (*create)(Inode *, Dentry *, const Attr *);  /* + permission */
	err_t (*unlink)(Inode *, Dentry *);  /* + permission */
	err_t (*mkdir) (Inode *, Dentry *);  /* + permission */
	err_t (*rmdir) (Inode *, Dentry *);  /* + permission */

	/* write inode to memory */
	err_t (*write)(Inode *);
	/* read inode with a given ino from memory */
	err_t (*read) (Inode *);
};

struct dentry_does {
	void (*nothing)(Dentry *);
};

/* ===== Default generics ==================================================== */

/**
 *  @return true if the file system, wants this inode to be deleted.
 */
static inline u8 generic_drop_inode(Inode *i) {
	return !i->i_nlink;
}
/**
 *  Provide sample method to hook on inode deletion...
 */
static inline err_t generic_delete_inode(Inode *i) {
	return E_NO_LOGIC;
}


/* ===== Functions ========================================================== */

/* allocate and initialize a new inode structure */
extern Inode *  iget(Super *, u32);
extern void     destroy_inode(Inode *);
/* drop usage count of an inode */
extern void     iput(Inode *);
/**
 * Lookup if an inode with given inode number is already in memory.
 */
extern Inode *  inode_find(Super *, u32);
/**
 * Fill an inode with data.
 */
extern err_t    inode_pull(Inode *);

static inline u8
inode_lcs_get(Inode *i) {
	return i->i_state & 0x0F;
}

static inline void
inode_lcs_set(Inode *i, u8 lcs) {
	i->i_state &= 0xF0;
	i->i_state |= 0x0F & lcs;
}

static inline u32
inode_smap_size(const Inode *i) {
	return i->i_sections * sizeof(struct section_map);
}


Dentry * dentry_lookup(Dentry *, fid_t);
Dentry * dentry_create(Dentry *, fid_t, Attr *attr);

/** drop usage of a dentry object */
void    dput(Dentry *);

/**
 * allocate a file object
 */
File   * fget(void);
/** release a file object */
err_t    fput(File *);

/** get the file object for this file descriptor */
File   * fd_lookup(u32);
/** get a file descriptor of a file object */
u32      fd_get(File *);

void     dentry_attach(Dentry *, Inode *);
Inode *  dentry_detach(Dentry *);

PUBLIC err_t simple_file_do_open(Inode *, File *);
PUBLIC err_t simple_file_do_release(Inode *, File *);

/**
 *  @return Dentry endpoint of path or NULL, if the object does not exist.
 */
PUBLIC err_t smartfs_path_lookup(const path_t, Dentry **);

PUBLIC err_t smartfs_mount_root( struct mem_dev *, fp_do_mount);

/**
 *  This method is just for debugging purpose.
 *
 *  The underlying data structures of file system management are initialized
 *  at system startup. During unit testing it is necessary to reset these
 *  structures (e.g. the inode array cache 'iarr')
 */
PUBLIC err_t smartfs_reset(void);

#endif
/* ----- end of macro protection _SMARTFS_H_ ----- */
