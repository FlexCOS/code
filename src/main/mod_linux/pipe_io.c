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
#include <unistd.h>
#include <string.h>

#include <const.h>
#include <types.h>

#include <array.h>
#include <buffers.h>
#include <modules.h>

#include "pipe_io.h"

#define R 0
#define W 1

PRIVATE int  pipe_card2term[2];
PRIVATE int  pipe_term2card[2];


PRIVATE const struct array *
pipe_card_read(void)
{
	array_truncate(__capdu);

	__capdu->length = read(pipe_term2card[R], __capdu->val, __capdu->max);

	return __capdu;
}

PRIVATE void
pipe_card_write(void)
{
	write(pipe_card2term[W], __rapdu->v, __rapdu->length);
	array_truncate(__rapdu);
}

PUBLIC size_t
pipe_term_read(buff8_t buff, size_t bytes)
{
	/* if card is in read mode we won't get any data, unfortunatly any
	 * reading attempt would cause a deadlock */
	return read(pipe_card2term[R], buff, bytes);
}

PUBLIC size_t
pipe_term_write(const buff8_t buff, size_t bytes)
{
	return write(pipe_term2card[W], buff, bytes);
}

PUBLIC err_t
pipe_io()
{
	static const struct module_io pio = {
		.receive  = pipe_card_read,
		.transmit = pipe_card_write,
	};

	if ((pipe(pipe_card2term) == -1) ||
	    (pipe(pipe_term2card) == -1)  )
	{
		return E_SYSTEM;
	}

	return module_hal_io_set(&pio);
}

