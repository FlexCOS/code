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

PUBLIC extern const struct super_does somefs_super_does;
PUBLIC extern const struct inode_does somefs_inode_does;

/* --- Interface: struct super_does --- */
PUBLIC Inode * somefs_super_do_alloc_inode(Super *);
PUBLIC void  somefs_super_do_destroy_inode(Inode *);
PUBLIC err_t somefs_do_read_inode(Inode *);
PUBLIC err_t somefs_do_write_inode(Inode *);

/* --- Interface: struct inode_does --- */
PUBLIC err_t somefs_inode_do_lookup(Inode *, Dentry *);
PUBLIC err_t somefs_inode_do_create(Inode *, Dentry *, const Attr *);

/* --- Interface: struct file_does --- */
PUBLIC size_t somefs_file_do_read(File *, buff8_t, size_t);
PUBLIC size_t somefs_file_do_write(File *, const buff8_t, size_t);
