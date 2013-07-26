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
    along with FlexCOS; if not itcan be viewed here:
    http://www.freertos.org/a00114.html and also obtained by writing to AGSI,
    contact details for whom are available on the FlexCOS WEB site.

*/

#include <const.h>
#include <types.h>
#include <string.h>
#include <array.h>

#include "stream.h"

PRIVATE struct stream_in_ops __array_stream_in_impl = {
	.get = array_stream__get,
	.read = NULL,
	.skip = NULL,
	.push_to = NULL
};
const struct stream_in_ops *const array_stream_in_impl = &__array_stream_in_impl;


PUBLIC u32
stream_skip_native(struct stream_in *is, u32 bytes)
{
	u8    dummy;
	u32   skipped;

	for (skipped = 0;
	     skipped < bytes && is->ops->get(is, &dummy);
	     skipped++);

	return skipped;
}

PUBLIC u32
stream_read_native(struct stream_in *is, u8 *buff, u32 bytes)
{
	u32 i;

	for (i = 0;
	     i < bytes && stream_get(is, buff + i);
	     i++);

	return i;
}

PUBLIC u32
stream_write_native(struct stream_out *os, u8 *buff, u32 bytes)
{
	if (!os->ops->put) return 0;

	u32 i;

	for (i = 0;
	     i < bytes && stream_put(os, buff[i]);
	     i++);

	return i;
}

PUBLIC u32
stream_transfer_native(struct stream_in *is, struct stream_out *os, u32 bytes)
{
	u8 c;

	while (bytes
	&&     stream_get(is, &c)
	&&     stream_put(os, c))
	{
		bytes--;
	}
	return bytes;
}

PUBLIC u32
array_stream__get(struct stream_in *is, u8 *c)
{
	struct array_stream_in *ais = (struct array_stream_in *) is;

	if (ais->bytes_left) {
		*c = ais->data[0];
		ais->data++;
		ais->bytes_left--;
		return 1;
	}

	return EOF;
}
