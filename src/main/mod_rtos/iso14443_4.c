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
 * iso14443_4.c
 *
 * Created:
 *     Author: Alexander Münn
 */

#include <const.h>
#include <types.h>

#include <string.h>

#include <array.h>

#include "iso14443_4.h"

PRIVATE inline void toggle_block_number(u8 *bn) { (*bn) ^= 0x01; }

enum {
	FWT_BASE = 309,  /* (256 * 16 / fc) = ~309us */
};


/**
 * Calculate frame waiting time for a given Frame Waiting time Integer (FWI).
 *
 * @return Frame Waiting Time in micro seconds, or zero on fwi > 14
 */
PUBLIC u32
iso14443_fwt_in_us(u8 fwi)
{
	/* precalculated fwt multiplier: {pow(2, FWI) | 0 <= FWI < 15} */
	static const u16 two_power[] = {
		   1,    2,    4,    8,
		  16,   32,   64,  128,
		 256,  512, 1024, 2048,
		4096, 8192,16384
	};

	if (fwi >= sizeof(two_power))
		return 0;

	return FWT_BASE * two_power[fwi];
}

/**
 * Parse a
 *
 * @return size of prologue field, or zero on format conflict
 */
PUBLIC u8
iso14443_parse_request(Block *request, const Array *const input, u8 *bn)
{
	/* we are going to iterate over input array */
	struct array_iterator block = { .arr = input, .pos = 0 };

	if (!array_iter_has_next(&block))
		return 0;

	request->pcb_plain = array_iter_next(&block);

	/* each PCB may have a following CID byte. */
	if (request->pcb.with_cid) {
		if (array_iter_has_next(&block))
			request->cid = array_iter_next(&block);
		else
			goto parse_error;
	}

	/* ------------------------------------------------------------------ */
	/* -- I-BLOCK HANDLING ---------------------------------------------- */
	/* ------------------------------------------------------------------ */
	if (is_iso14443_i_block(request->pcb_plain)) {
		/* toggle block number on every I-Block */
		toggle_block_number(bn);
		/* NAD is not supported */
		if (request->pcb.with_nad) goto parse_error;

		/* on empty I-block send empty I-block back and leave */
		if (!array_iter_has_next(&block)) {
			request->type = I_BLK_PRESENCE;
			goto exit;
		}
		else if (request->pcb.type == I_BLK_C) {
			request->type = CAPDU_TRANSFER;
		}
		else {
			request->type = CAPDU_COMPLETE;
		}
		/* Otherwise there is some INF data available. */
		request->INF      = &(block.arr->v[block.pos]);
		request->INF_size = block.arr->length - block.pos;
	}
	/* ------------------------------------------------------------------ */
	/* -- R-BLOCK HANDLING ---------------------------------------------- */
	/* ------------------------------------------------------------------ */
	else if (is_iso14443_r_block(request->pcb_plain)) {
		if (request->pcb.block_number == (*bn)) {
			request->type = R_BLK_RESEND;
		}
		/* PCD confirms received I-block */
		else if (request->pcb.type == R_ACK) {
			toggle_block_number(bn);
			/* continue chaining */
			request->type = RAPDU_CONTINUE;
		}
		/* PCD performs presence check */
		else {
			request->type = R_NAK_PRESENCE;
		}
	}
	/* ------------------------------------------------------------------ */
	/* -- S-BLOCK HANDLING ---------------------------------------------- */
	/* ------------------------------------------------------------------ */
	else if (is_iso14443_s_block(request->pcb_plain)) {
		request->type = S_WTX_CONFIRM;
	}
	else goto parse_error;

exit:
	return block.pos;

parse_error:
	request->type = INVALID;
	return 0;
}
