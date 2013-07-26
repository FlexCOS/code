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
#include <i7816.h>
#include <flxio.h>
#include <apdu.h>
#include <channel.h>
#include <io/stream.h>
#include <io/file_stream.h>

PUBLIC sw_t
cmd_read_record__current_ef(const CmdAPDU *capdu)
{
	struct file_stream_in ef_stream;
	FILE  *ef = current->ef;
	u32   transfered;
	err_t err;

	if (!capdu->Le) return SW__WRONG_LE;

	if (!ef) return SW__NOT_ALLOWED;

	/* TODO check Bit3 of P2 first */
	if (capdu->header->P1) {
		err = f_seeks(ef, capdu->header->P1, SEEK_SET);
		if (err) return SW__RECORD_NOT_FOUND;
	}

	/* Read explicitly from the beginning */
	if (f_seek(ef, 0, SEEK_SET)) return 0xDEAD;

	file_stream_in_init(&ef_stream, ef);

	transfered = stream_transfer(&ef_stream.stream, current->response, capdu->Le);

	if (transfered == capdu->Le)
		return SW__OK;
	else
		return 0x0001;
}

PUBLIC sw_t
cmd_read_record__with_sfi(const CmdAPDU *capdu)
{
	return SW__FUNCTION_NOT_SUPPORTED;
}
