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

#include <stdlib.h>
#include <string.h>

#include "const.h"
#include "types.h"

#include <io/dev.h>
#include <array.h>

#include "modules.h"

PRIVATE const struct array * mod_io_noop_read(void);
PRIVATE void mod_io_noop_write(void);

PRIVATE struct module_io io = {
	.receive  = mod_io_noop_read,
	.transmit = mod_io_noop_write,
};

PRIVATE struct mem_dev dev = {
	.read = NULL,
	.write = NULL,
	.size = 0
};

PRIVATE const struct array *
mod_io_noop_read(void) 
{
	return NULL;
}
PRIVATE void
mod_io_noop_write(void)
{
	return;
}

/* module access from outside */
PUBLIC const struct module_io *const hal_io = &io;

PUBLIC struct mem_dev *const hal_mdev = &dev;

PUBLIC err_t
modules_init(void)
{
	u8 n = _nmods;
	u8 i;
	err_t ret;

	ret = E_GOOD;

	for (i = 0; i < n && ret == E_GOOD; i++) {
		ret = modules[i]();
	}

	return ret;
}

PUBLIC err_t
module_hal_io_set(const struct module_io *_io)
{
	static bool initialized = false;

	/* there must be only one call */
	if (initialized) return E_SYSTEM;

	io.receive  = _io->receive;
	io.transmit = _io->transmit;

	initialized = true;

	return E_GOOD;
}

PUBLIC err_t
module_hal_mdev_set(const struct mem_dev *_dev)
{
	static bool initialized = false;

	/* there must be only one call */
	if (initialized) return E_SYSTEM;

	CHECK_PARAM__NOT_NULL(_dev);
	CHECK_PARAM__NOT_NULL(_dev->read);
	CHECK_PARAM__NOT_NULL(_dev->write);
	CHECK_PARAM__NOT_ZERO(_dev->size);

	dev = *_dev;

	initialized = true;

	return E_GOOD;
}
