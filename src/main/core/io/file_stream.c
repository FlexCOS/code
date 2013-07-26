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
#include <flxio.h>
#include <io/stream.h>
#include "file_stream.h"


static inline struct file_stream_in *
__to_fis(struct stream_in *is)
{
	return stream_type(is, struct file_stream_in, stream);
}

PRIVATE u32
file_stream_read(struct stream_in *is, u8 *buff, u32 bytes)
{
	struct file_stream_in *fis = __to_fis(is);

	return f_read(buff, 1, bytes, fis->fh);
}

PRIVATE u32
file_stream_get(struct stream_in *is, u8 *c)
{
	return file_stream_read(is, c, 1);
}

PRIVATE u32
file_stream_skip(struct stream_in *is, u32 bytes)
{
	return 0;
}

PRIVATE const struct stream_in_ops __file_stream_in_ops = {
	.get = file_stream_get,
	.read = file_stream_read,
	.skip = file_stream_skip,
	.push_to = NULL
};

PUBLIC err_t
file_stream_in_init(struct file_stream_in *fis, FILE *fh)
{
	fis->stream.ops = &__file_stream_in_ops;

	fis->fh = fh;
	return E_GOOD;
}
