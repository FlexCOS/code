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

#include <const.h>
#include <types.h>
#include <string.h>
#include <array.h>
#include <modules.h>
#include "buffers.h"

// XXX
// ifdef LINUX_foo
/*#include <unistd.h>
#define  wait_a_moment sleep(1)
*/
#define wait_a_moment {}
// endif



PUBLIC u16
local_capdu_send(const u8 *data, u16 bytes)
{
	// if (__capdu->length) return 0;
	return array_append(__capdu, data, bytes);
}

/**
 *  @return number of bytes that have been received from card OS.
 */
PUBLIC u16
local_rapdu_recv(u8 *buff, u16 max)
{
	u16 bytes = MIN(max, __rapdu->length);
	u16 i;

	for_each(i, 0, 3)
		if (!__rapdu->length) wait_a_moment;

	memcpy(buff, __rapdu->val, bytes);

	/* remove copied bytes, but move remaining bytes to 
	 * beginning of __rapdu: similar to left-shift operation
	 * ...or right-shift depending on your view point. */
	for_each(i, 0, (__rapdu->length - bytes))
		__rapdu->val[i] = __rapdu->val[bytes+i];

	memset(__rapdu->val + bytes, 0x00, __rapdu->max - bytes);

	__rapdu->length -= bytes;

	return bytes;
}

/* Implement hal_io interface */


PRIVATE const struct array *
local_capdu_recv(void)
{
	/* XXX improve waiting */
	while (!__capdu->length) wait_a_moment;

	array_reset(__rapdu);

	return __capdu;
}

PRIVATE void
local_rapdu_transmit(void)
{
	array_reset(__capdu);

	return;
}

PUBLIC err_t
local_io(void)
{
	struct module_io io = {
		.receive  = local_capdu_recv,
		.transmit = local_rapdu_transmit
	};

	array_clean(__capdu);
	array_clean(__rapdu);

	return module_hal_io_set(&io);
}
