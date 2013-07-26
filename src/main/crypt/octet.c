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
#include <string.h>

#include "octet.h"
/**
 * Fill up an octet up to its maximum capacity with value 'v'.
 *
 * @return Number of bytes that have been filled.
 */
PUBLIC oltype
octet_fill(octet *o, u8 v)
{
	oltype bytes = octet_bytes_left(o);

	memset(octet_end(o), v, bytes);

	o->len = o->max;

	return bytes;
}

PUBLIC oltype
octet_fill_random(octet *o, oltype bytes)
{
	return 0;
}

PUBLIC void
octet_release(octet *o)
{
	if (o->val) free(o->val);

	memset(o, 0x00, sizeof(*o));
}

