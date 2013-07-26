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
 * apdu.c
 *
 * Created
 *     Author: Alexander Münn
 */

#include <flxlib.h>
#include <apdu.h>

/** 
 *  Fill an command APDU object
 */
PUBLIC err_t
apdu_validate_cmd(CmdAPDU *capdu)
{
	u16 Lx, pos;
	CHECK_PARAM__NOT_NULL (capdu);
	CHECK_PARAM__NOT_NULL (capdu->msg);

	capdu->__Le   = 0;
	capdu->__Lc   = 0;
	capdu->__data = NULL;

	if (capdu->length < 4)
		return E_APDU_TOO_SHORT;
	
	if (capdu->length == 4)
		return E_GOOD;
	/* We now that there is a four byte apdu header:
	 * Jump to the end of it and test for fourther bytes. The next field
	 * must be an Lx value, either Lc or Le. */
	pos = 4;
	Lx = capdu->msg[pos++];
	/* A value of 0x00 denotes an extended length field, if the command
	 * APDU has two more bytes. */
	if (Lx == 0x00 && capdu->length >= pos + 2) {
		Lx  = capdu->msg[pos++] << 8;
		Lx |= capdu->msg[pos++];
		/* extended length encoding MUST NOT be zero */
		if (Lx == 0x0000) {
		/* FIXME valid for Lc, but this bans 0x0000 special case for Le */
			return E_APDU_LENGTH;
		}
	}
	/* If there is no unprocessed data left, we are done here, and the Lx 
	 * value is interpreted as Le value. */
	if (pos == capdu->length) {
		/* handle special case: one byte 0x00 is Le of 256 byte */
		capdu->__Le = Lx ? Lx : 256;
		return E_GOOD;
	} else {
		capdu->__Lc = Lx;
	}

	if ( !capdu->Lc                          || 
	     (capdu->Lc + pos > capdu->length)   ||
	     (capdu->Lc + pos < capdu->length - 2))
	{
		return E_APDU_LC;
	}

	/* we are no safe to reference data part and jump
	 * to the end of it */
	capdu->__data = &(capdu->msg[pos]);
	pos += capdu->Lc;

	if (capdu->length == pos)
		return E_GOOD;

	capdu->__Le = capdu->msg[pos++];
	if (pos < capdu->length) {
		capdu->__Le <<= 8;
		capdu->__Le  |= capdu->msg[pos];

		/* XXX special case 0x0000 => 0xFFFF + 1 */
	} else {
		/* handle special case: one byte 0x00 is Le of 256 byte */
		capdu->__Le = capdu->Le ? capdu->Le : 256;
	}

	return capdu->Le ? E_GOOD : E_APDU_LE;
}
