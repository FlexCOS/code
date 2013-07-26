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
#include <const.h>
#include <types.h>
#include <string.h>


#include "array.h"

PUBLIC u16
array_append(Array *arr, const u8 *data, u16 bytes)
{
	u16 cpy = MIN(bytes, array_bytes_left(arr));

	memcpy(array_end(arr), data, cpy);

	arr->length += cpy;

	return cpy;
}

PUBLIC u8
array_put(Array *arr, u8 byte)
{
	if (array_bytes_left(arr)) {
		arr->val[arr->length++] = byte;
		return 1;
	}
	return 0;
}

PUBLIC void
array_clean(Array *arr)
{
	arr->length = 0;
	memset(arr->val, 0x00, arr->max);
}

PUBLIC void
array_copy(const struct array *from, struct array *to)
{
	u16 bytes = MIN(from->len, to->max);

	array_clean(to);

	memcpy(to->val, from->val, bytes);
	to->len = bytes;
	return;
}

/**
 * Fill an array up to its maximum capacity with value 'v'.
 *
 * @return Number of bytes that have been filled.
 */
PUBLIC u16
array_fill(array *a, u8 v)
{
	u16 bytes = array_bytes_left(a);

	memset(array_end(a), v, bytes);

	a->len = a->max;

	return bytes;
}

PUBLIC struct array *
array_alloc(u16 bytes)
{
	struct array *new = malloc(sizeof(*new) + bytes);
	new->length = 0;
	new->__max = bytes;
	new->__val = (typeof(new->val)) (new + 1);

	return new;
}

PUBLIC void
array_free(struct array *arr)
{
	free(arr);
	return;
}
