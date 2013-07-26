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
#include <CUnit/Basic.h>
#include <fs/smartfs.h>

#include <common/test_macros.h>
#include <common/test_utils.h>

#include "stub_fs.h"

static int init_suite();
static int clean_suite();

PRIVATE bool is_initialized = false;
PRIVATE const fid_t TEST_FILE_FID = 0x3300;
PRIVATE const fid_t TEST_DIR_FID = 0xdead;
PRIVATE const Attr  TEST_FILE_ATTR = { .sections = 1, .sec_size = 4 };
PRIVATE Inode  *test_file_inode  = NULL;
PRIVATE Dentry *test_dentry = NULL;
PRIVATE u32 test_ino = 0;

PRIVATE void test_initialized(void);
PRIVATE void test_inode_single_get_put(void);
PRIVATE void test_inode_max_get_put(void);

PRIVATE void test_path_lookup__file(void);
PRIVATE void test_path_lookup__subdir(void);
PRIVATE void test_path_lookup__MF(void);
PRIVATE void test_path_lookup__EOP(void);

// TODO rework:
PRIVATE void test_super_do_write_and_read_inode(void);
PRIVATE void test_super_do_delete_inode(void);
PRIVATE void test_inode_do_create(void);
PRIVATE void test_inode_do_mkdir(void);
PRIVATE void test_inode_do_read(void);
PRIVATE void test_inode_do_write(void);
PRIVATE void test_inode_do_unlink(void);
PRIVATE void test_inode_do_rmdir(void);
PRIVATE void test_file_do_open_and_release(void);
PRIVATE void test_file_do_write(void);
PRIVATE void test_file_do_read(void);
// XXX end of rework

PRIVATE void sub_test_what_a_file_does(Inode *);
PRIVATE void sub_test_what_a_dir_does(Inode *);
PRIVATE void sub_test_initialized_dentry(Inode*, Dentry *);

PRIVATE void sub_test_inode_initialized(Super*, Inode *);
PRIVATE void sub_test_inode_deleted(Inode *);


static const struct test_case tc_arr[] = {
	TEST_CASE ( test_initialized, "is initialized" ),
	TEST_CASE ( test_inode_single_get_put, "allocate and destroy a single inode" ),
	TEST_CASE ( test_inode_max_get_put, "maximum inode allocation"),
	TEST_CASE ( test_inode_do_create, "inode - create" ),
	TEST_CASE ( test_inode_do_read, "inode - read" ),
	TEST_CASE ( test_inode_do_write, "inode - write" ),
	TEST_CASE ( test_inode_do_mkdir, "inode - mkdir" ),
	TEST_CASE ( test_path_lookup__MF, "path - lookup master file" ),
	TEST_CASE ( test_path_lookup__file, "path - lookup file" ),
	TEST_CASE ( test_path_lookup__EOP,  "path - lookup empty path" ),
	TEST_CASE ( test_path_lookup__subdir, "path - lookup subdir" ),
	TEST_CASE ( test_super_do_write_and_read_inode, "super - write and read inode"),
	TEST_CASE ( test_inode_do_unlink, "inode - unlink"),
	TEST_CASE ( test_super_do_delete_inode, "super - delete inode" ),
	TEST_CASE ( test_inode_do_rmdir, "inode - rmdir" ),
	TEST_CASE ( test_file_do_write, "file - write" ),
	TEST_CASE ( test_file_do_read, "file - read" ),
};

int build_suite__smartfs()
{
	INIT_BUILD_SUITE();
	CU_pSuite pSuite = NULL;

	CREATE_SUITE_OR_DIE("SmartFS core", pSuite);
	ADD_TEST_CASES_OR_DIE(pSuite, tc_arr);

	return 0;
}

/* The suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
static int
init_suite(void)
{
	return stub_fs_init();
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
static int
clean_suite(void)
{
	return stub_fs_free();
}

/* ===========================================================================*
 *  Local subtest implementations
 * ========================================================================== */

PRIVATE void
sub_test_initialized_dentry(Inode *iparent, Dentry* d)
{
	CU_ASSERT_PTR_NOT_NULL_FATAL ( d->d_sb );
	CU_ASSERT_PTR_NOT_NULL_FATAL ( d->d_inode );
	CU_ASSERT_PTR_NOT_NULL_FATAL ( d->d_inode->i_do );

	CU_ASSERT_PTR_EQUAL ( d->d_sb,            iparent->i_super );
	CU_ASSERT_PTR_EQUAL ( d->d_inode->i_mdev, iparent->i_mdev );
	CU_ASSERT_PTR_EQUAL ( d->d_inode->i_super,iparent->i_super );
}

PRIVATE void
sub_test_inode_initialized(Super *s, Inode *i)
{
	CU_ASSERT_PTR_NOT_NULL_FATAL (s);

	CU_ASSERT_PTR_NOT_NULL (i->i_do);
	CU_ASSERT_PTR_NOT_NULL (i->i_fdo);
	CU_ASSERT_PTR_NOT_NULL (i->i_mdev);

	CU_ASSERT_PTR_EQUAL (s,         i->i_super);
	CU_ASSERT_PTR_EQUAL (s->s_mdev, i->i_mdev);

	CU_ASSERT_NOT_EQUAL (i->i_ino,  0);
	CU_ASSERT_NOT_EQUAL (i->i_count,0);
}

/*
 * A deleted inode structure should be set to zero.
 */
PRIVATE void
sub_test_inode_deleted(Inode *i)
{
	CU_ASSERT_EQUAL    (i->i_count, 0);
	CU_ASSERT_EQUAL    (i->i_ino, 0);
	CU_ASSERT_PTR_NULL (i->i_do);
	CU_ASSERT_PTR_NULL (i->i_fdo);
	CU_ASSERT_PTR_NULL (i->i_super);
	CU_ASSERT_PTR_NULL (i->i_mdev);
	CU_ASSERT_PTR_NULL (i->i_ctx);
}

/*
 * Mandatory file system implementation of file_does interface for file (EF).
 */
PRIVATE void
sub_test_what_a_file_does(Inode *file)
{
	CU_ASSERT_PTR_NOT_NULL_FATAL (file);

	CU_ASSERT_PTR_NOT_NULL ( file->i_fdo );
	CU_ASSERT_PTR_NOT_NULL ( file->i_fdo->read );
	CU_ASSERT_PTR_NOT_NULL ( file->i_fdo->write );

	CU_ASSERT_PTR_NULL ( file->i_fdo->readdir );
}

/*
 * Mandatory file system implementation of file_does interface for directories.
 */
PRIVATE void
sub_test_what_a_dir_does(Inode *dir)
{
	CU_ASSERT_PTR_NOT_NULL_FATAL (dir);
	if (!dir->i_fdo) {
		CU_FAIL ("no file operations for [dir]");
	}
	else {
		CU_ASSERT_PTR_NOT_NULL ( dir->i_fdo->readdir );
		/* there must not be a file write operation directories */
		CU_ASSERT_PTR_NULL ( dir->i_fdo->write );
	}
}

/* ===========================================================================*
 *  Test case implementations
 * ========================================================================== */

/**
 *  An additional check for smartfs requirements...
 */
PRIVATE void
test_initialized(void)
{
	CU_ASSERT_PTR_NOT_NULL_FATAL (mnt.droot);
	CU_ASSERT_PTR_NOT_NULL_FATAL (mnt.super);
	CU_ASSERT_PTR_NOT_NULL_FATAL (mnt.super->s_do);
	CU_ASSERT_PTR_NOT_NULL_FATAL (mnt.droot->d_inode);
	CU_ASSERT_PTR_NOT_NULL_FATAL (mnt.droot->d_inode->i_do);
	CU_ASSERT_PTR_NOT_NULL_FATAL (mnt.droot->d_inode->i_fdo);
	CU_ASSERT_PTR_NOT_NULL_FATAL (mnt.super->s_mdev);

	CU_ASSERT_EQUAL_FATAL (mnt.super, mnt.droot->d_sb);
	CU_ASSERT_EQUAL_FATAL (mnt.super, mnt.droot->d_inode->i_super);

	is_initialized = true;
}

/**
 *  This test uses file system implementation to allocate and destroy inodes.
 */
PRIVATE void
test_inode_single_get_put(void)
{
	Super *const s = mnt.super;
	Inode *i;

	CU_ASSERT_TRUE_FATAL (is_initialized);
	i = iget(s, 1);
	CU_ASSERT_PTR_NOT_NULL_FATAL (i);

	sub_test_inode_initialized(s, i);
	CU_ASSERT_EQUAL (i->i_ino, 1);

	CU_ASSERT_TRUE  (i->i_state & I_NEW);

	iput(i);
	/* There should be no reference left. Which causes the inode to be
	 * destroyed. */
	sub_test_inode_deleted(i);
}

/**
 *  This test targets boundaries of inode allocation.
 *
 *  NOTE! As the file system is mounted, one inode from allocation
 *  pool is already in use.
 */
PRIVATE void
test_inode_max_get_put()
{
	Super *super = mnt.super;
	Inode *inodes[FS_MAX_ACTIVE_INODES];
	Inode *inode;
	u32   i;

	for (i = 0; i < (FS_MAX_ACTIVE_INODES - 1); i++) {
		inodes[i] = iget(super, i);
		CU_ASSERT_PTR_NOT_NULL( inodes[i] );
		if (inodes[i])
			CU_ASSERT_EQUAL ( inodes[i]->i_count, 1 );
		// TODO test of duplication
	}

	inode = iget(super, i);
	CU_ASSERT_PTR_NULL ( inode );
	if (inode) {
		CU_FAIL ( "Could allocate more inodes than expected!" );
		iput (inode);
	}

	for (i = 0; i < (FS_MAX_ACTIVE_INODES - 1); i++) {
		iput(inodes[i]);
		sub_test_inode_deleted(inodes[i]);
	}
}

/**
 * Create a single file
 */
PRIVATE void
test_inode_do_create(void)
{
	static Dentry dentry1 = {0};
	static Dentry dentry2 = {0};
	Dentry *d, *d2;
	Inode  *const iparent = mnt.droot->d_inode;
	const struct inode_does *iparent_do = iparent->i_do;
	const Attr *const attr = &TEST_FILE_ATTR;
	err_t  err;

	CU_ASSERT_TRUE_FATAL (is_initialized);
	CU_ASSERT_PTR_NOT_NULL_FATAL (iparent->i_do->create);

	err = iparent_do->create(iparent, NULL, attr );
	CU_ASSERT_TRUE (err);

	d  = &dentry1;
	d2 = &dentry2;
	test_dentry = NULL;

	err = iparent_do->create(iparent, d, attr );
	CU_ASSERT_TRUE (err);

	d->d_name = TEST_FILE_FID;
	d2->d_name = TEST_FILE_FID;

	err = iparent_do->create(iparent, d, attr );
	CU_ASSERT_EQUAL (err, E_GOOD );

	err = iparent_do->create(iparent, d2, attr );
	CU_ASSERT_TRUE (err);

	sub_test_initialized_dentry( iparent, d );

	CU_ASSERT_EQUAL ( d->d_inode->i_size, attr->sec_size * attr->sections );
	CU_ASSERT_EQUAL ( d->d_inode->i_sections, attr->sections );

	test_dentry = d;
	test_file_inode = d->d_inode;
	test_ino    = d->d_inode->i_ino;
}

PRIVATE void
test_inode_do_mkdir(void)
{
	CU_FAIL ( "Not implemented" );
}


/**
 * What do we expect from an EOP-path lookup? Application-Root?
 */
PRIVATE void
test_path_lookup__EOP(void)
{
	CU_FAIL ( "Not implemented" );
}

/**
 * Simple root directory lookup.
 *
 * This test focuses on correct reference count after lookup.
 */
PRIVATE void
test_path_lookup__MF(void)
{
	Dentry *dentry = NULL;
	fid_t  path[] = { MF, EOP };
	err_t  err;

	CU_ASSERT_TRUE_FATAL (is_initialized);

	err = smartfs_path_lookup(path, &dentry);
	CU_ASSERT_EQUAL_FATAL (err, E_GOOD);
	CU_ASSERT_PTR_NOT_NULL (dentry);
	/* since the root dentry has a default reference in smartfs, the
	 * lookup must have incremented the reference counter */
	CU_ASSERT_EQUAL (dentry->d_count, 2);

	CU_ASSERT_PTR_EQUAL (dentry, mnt.droot);
	sub_test_what_a_dir_does(dentry->d_inode);

	dput(dentry);
	CU_ASSERT_EQUAL (mnt.droot->d_count, 1);
}

PRIVATE void
test_path_lookup__file(void)
{
	Dentry *dentry;
	Dentry *droot = mnt.droot;
	Inode  *iroot = mnt.droot->d_inode;
	err_t  err;
	fid_t  path[] = { TEST_FILE_FID, EOP };

	CU_ASSERT_TRUE_FATAL (is_initialized);
	CU_ASSERT_PTR_NOT_NULL_FATAL (iroot->i_do->lookup);

	err = smartfs_path_lookup(path, &dentry);
	CU_ASSERT_EQUAL_FATAL (err, E_GOOD);
	CU_ASSERT_PTR_NOT_NULL (dentry);
	sub_test_initialized_dentry(iroot, dentry );
	sub_test_inode_initialized (iroot->i_super, dentry->d_inode);
	sub_test_what_a_file_does(dentry->d_inode);

	/* there is no reference to root directory */
	CU_ASSERT_EQUAL (droot->d_count,  1);
	CU_ASSERT_EQUAL (dentry->d_count, 1);

	dput(dentry);
}

PRIVATE void
test_path_lookup__subdir(void)
{
	Dentry *dentry;
	Inode  *iroot = mnt.droot->d_inode;
	err_t  err;
	fid_t  path[] = { TEST_DIR_FID, EOP };

	CU_ASSERT_TRUE_FATAL (is_initialized);
	CU_ASSERT_PTR_NOT_NULL_FATAL (iroot->i_do->lookup);

	err = smartfs_path_lookup(path, &dentry);

	CU_ASSERT_EQUAL_FATAL (err, E_GOOD);
	CU_ASSERT_PTR_NOT_NULL (dentry);
	sub_test_initialized_dentry(iroot, dentry );

	sub_test_inode_initialized (iroot->i_super, dentry->d_inode);

	sub_test_what_a_dir_does(dentry->d_inode);

	dput(dentry);
}

PRIVATE void
test_super_do_write_and_read_inode(void)
{
	Inode *i;
	Super *const s = mnt.super;
	err_t err;

	CU_ASSERT_TRUE_FATAL (is_initialized);
	CU_ASSERT_PTR_NOT_NULL_FATAL (s->s_do->read_inode);

	if (test_ino == 0) {
		CU_FAIL_FATAL ("depend on test_inode_do_create");
	}

	i = iget(s, test_ino);
	CU_ASSERT_PTR_NOT_NULL_FATAL (i);

	err = inode_pull(i);
	CU_ASSERT_EQUAL (err, E_GOOD);

	iput(i);
}


PRIVATE void
test_super_do_delete_inode(void)
{
	CU_FAIL ( "Not implemented");
}

PRIVATE void
test_inode_do_read(void)
{
	CU_FAIL ( "Not implemented");
}

PRIVATE void
test_inode_do_write(void)
{
	CU_FAIL ( "Not implemented");
}

PRIVATE void
test_inode_do_unlink()
{
	CU_FAIL ( "Not implemented" );
}

PRIVATE void
test_inode_do_rmdir()
{
	CU_FAIL ( "Not implemented" );
}

PRIVATE void __deprecated
test_file_do_open_and_release(void)
{
	const struct file_does *file_do = test_file_inode->i_fdo;
	File  f = { .f_dentry = NULL, .f_do = NULL };
	File  f2;
	err_t err;
	CU_ASSERT_TRUE_FATAL (is_initialized);
	CU_ASSERT_PTR_NOT_NULL_FATAL (test_file_inode);

	// XXX test with invalid inode?

	err = file_do->open(test_file_inode, &f);
	CU_ASSERT_EQUAL (err, E_GOOD);
	CU_ASSERT_EQUAL (f.pos, 0);
	CU_ASSERT_PTR_NOT_NULL (f.f_do);
	CU_ASSERT_PTR_NOT_NULL (f.f_dentry);

	/* do not reopen */
	f2 = f;
	err = file_do->open(test_file_inode, &f2);
	CU_ASSERT_TRUE (err);

	err = file_do->release(test_file_inode, &f);
	CU_ASSERT_PTR_NULL (f.f_do);
	CU_ASSERT_PTR_NULL (f.f_dentry);
}

PRIVATE void
test_file_do_write(void)
{
	const struct file_does *file_do = test_file_inode->i_fdo;
	File   f = { .f_dentry = NULL };
	err_t  err;
	size_t written;

	CU_ASSERT_TRUE_FATAL (is_initialized);
	CU_ASSERT_PTR_NOT_NULL_FATAL (test_file_inode);

	/* generic fill file pointer */
	f.f_dentry = test_dentry;
	test_dentry->d_count++;
	f.f_do = file_do;
	f.section = 0;
	f.pos = 0;

	/* open call to file system is optional */
	if (file_do->open) {
		err = file_do->open(test_file_inode, &f);
		CU_ASSERT_EQUAL_FATAL (err, E_GOOD);
		/* Test against any bad modifications */
		CU_ASSERT_EQUAL (f.pos, 0);
		CU_ASSERT_PTR_EQUAL (f.f_dentry->d_inode, test_file_inode);
	} else {
		CU_PASS ( "file_does->open is not available");
	}

	CU_ASSERT_PTR_NOT_NULL_FATAL (f.f_do->write);
	written = file_do->write((&f), SRC(&OxDEADBEAF.get), 4);
	CU_ASSERT_EQUAL (written, 4);
	CU_ASSERT_EQUAL (f.section, 0);
	CU_ASSERT_EQUAL (f.pos, written);

	if (file_do->release) {
		err = file_do->release(test_file_inode, (&f));
		CU_ASSERT_EQUAL (err, E_GOOD);
	}
}

PRIVATE void
test_file_do_read(void)
{
	const struct file_does *file_do = test_file_inode->i_fdo;
	File   f = {0};
	err_t  err;
	union blob_u32 blob;
	size_t r;

	CU_ASSERT_TRUE_FATAL (is_initialized);
	CU_ASSERT_PTR_NOT_NULL_FATAL (test_file_inode);
	/* generic fill file pointer */
	f.f_dentry = test_dentry;
	test_dentry->d_count++;
	f.f_do = file_do;
	f.section  = 0;
	f.pos  = 0;

	/* open the test file */
	if (file_do->open) {
		err = file_do->open(test_file_inode, &f);
		CU_ASSERT_EQUAL_FATAL (err, E_GOOD);
		CU_ASSERT_EQUAL (f.pos, 0);
		CU_ASSERT_PTR_EQUAL (f.f_dentry->d_inode, test_file_inode);
		CU_ASSERT_PTR_NOT_NULL_FATAL (f.f_do);
	} else {
		CU_PASS ( "file_does->open is not available" );
	}

	CU_ASSERT_PTR_NOT_NULL_FATAL (f.f_do->read);

	r = file_do->read((&f), DEST(&blob), 4);
	CU_ASSERT_EQUAL (r, 4);
	CU_ASSERT_EQUAL (blob.get, OxDEADBEAF.get );
	CU_ASSERT_EQUAL (f.section, 0);
	CU_ASSERT_EQUAL (f.pos, r);

	r = file_do->read((&f), DEST(&blob.head16), 2);
	CU_ASSERT_EQUAL (r, EOF);

	if (file_do->release) {
		err = file_do->release(test_file_inode, (&f));
		CU_ASSERT_EQUAL (err, E_GOOD);
	} else {
		CU_PASS ( "file_does->release is not available!" );
	}
}

