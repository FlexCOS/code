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

/*
 * common/buffers.c
 *
 *  Created on: Jul 19, 2012
 *      Author: Alexander Münn
 */
#include <flxlib.h>
#include <io/stream.h>

#include <string.h>
#include <stdlib.h>

#include <array.h>

#include "buffers.h"

// FIXME remove magic numbers
unsigned char apdu_input_buffer[256*16];
unsigned char apdu_output_buffer[256*16];

PRIVATE Array __capdu_data = Array(apdu_input_buffer, sizeof(apdu_input_buffer));
PRIVATE Array __rapdu_data = Array(apdu_output_buffer, sizeof(apdu_output_buffer));

PUBLIC Array *const __capdu = &__capdu_data;
PUBLIC Array *const __rapdu = &__rapdu_data;

/**
 * @Implements .put of 'struct stream_out'
 */
PRIVATE u32
apdu_response_put(struct stream_out *ctx, u8 c)
{
	return array_put(&__rapdu_data, c);
}

PRIVATE u32
apdu_response_fetch(struct stream_out *os, struct stream_in *is, u32 bytes)
{
	u32 max_bytes = MIN(bytes, array_bytes_left(&__rapdu_data));

	u32 written = stream_read(is, array_end(&__rapdu_data), max_bytes);

	__rapdu_data.length += written;

	return written;
}

PRIVATE struct stream_out_ops _apdu_response_ops = {
	.put = apdu_response_put,
	.fetch_from = apdu_response_fetch
};
PRIVATE struct stream_out _apdu_response = {
	.ops = &_apdu_response_ops
};

struct stream_out *const apdu_response = &_apdu_response;
