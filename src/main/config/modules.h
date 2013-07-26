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

/* Definition of (non-volatile) memory device is extern */
struct mem_dev;

/** 
 * Define each function 'f: () -> err_t' as a intialization function for
 * an arbitrary module. One may register these functions in modules array.
 * By calling modules_init() each registered module function gets called.
 */
typedef err_t (*fp_init_module)(void);

/**
 *  Register a set of initialization functions (modules).
 */
#define MODULES(mods...) const fp_init_module modules[] = { mods }; \
                         const u8 _nmods = LENGTH(modules);

/**
 *  
 */
extern const fp_init_module modules[];
/**
 * We need an explicit number of registered modules.
 */
extern const u8             _nmods;

const struct module_io *const hal_io;

/* FIXME make structure unmodifiable. Adding const will cause many conflicts
 * with filesystem design. */
struct mem_dev   *const hal_mdev;

struct module_io {
	const struct array *(*receive)(void);
	void                (*transmit)(void);
};

err_t module_hal_io_set(const struct module_io *);

err_t module_hal_mdev_set(const struct mem_dev *);

/** 
 *  Call 
 */
PUBLIC err_t modules_init(void);
