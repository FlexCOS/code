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

/**
 * flexcos.c
 *
 * Created
 *     Author: Alexander Münn
 */

#include "flxlib.h"

#include <array.h>
#include <buffers.h>
#include <modules.h>

#include "apdu.h"
#include "channel.h"

#include "io/stream.h"


/**
 * Lookup a command handler from 'registry' and call them.
 */
PRIVATE inline sw_t
handle(CmdAPDU *c)
{
	return apdu_get_cmd_handler(c)(c);
}

/** 
 *
 */
PUBLIC void
flexcos_process(const struct array *arr, struct stream_out *os)
{
	sw_t    sw;
	CmdAPDU capdu   = {
		.msg    = arr->val,
		.length = arr->length
	};
	/* XXX static channel */
	current->response = apdu_response;

	/* check command for length fields */
	if (apdu_validate_cmd(&capdu)) {
		/* write error */
		stream_put_word(current->response, SW__WRONG_LENGTH);
		return;
	}

	/** TODO handle channel */
	/** TODO handle MSE */

	sw = handle(&capdu);

	stream_put_word(current->response, sw);
}

PUBLIC void
flexcos_one_shot(void)
{
	const Array *capdu_raw;
	capdu_raw = hal_io->receive();
	flexcos_process(capdu_raw, apdu_response);
	// FIXME unobvious relation between apdu_response stream
	// and transmission function
	hal_io->transmit();
}

PUBLIC void
flexcos_run(void)
{
	loop {
		flexcos_one_shot();
	}
}
