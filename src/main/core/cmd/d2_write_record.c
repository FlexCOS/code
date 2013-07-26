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

PUBLIC sw_t
cmd_write_record__current_ef(const CmdAPDU *capdu)
{
	err_t  err;
	size_t written;
	FILE   *ef = current->ef;

	if (capdu->Le) return SW__WRONG_LE;

	if (!ef) return SW__NO_EF;

	/* jump to section */
	if (capdu->header->P2 & 0x04) {
		err = f_seeks(ef, capdu->header->P1, SEEK_SET);
	} else switch (capdu->header->P2 & 0x03) {
	/* First Record */
	case 0x00:
		err = f_seeks(ef, 0, SEEK_SET);
		break;
	/* Last Record */
	case 0x01:
		err = f_seeks(ef, 0, SEEK_END);
		break;
	/* Next Record */
	case 0x02:
		err = f_seeks(ef, 1, SEEK_CUR);
		break;
	/* Previous Record */
	case 0x03:
		err = f_seeks(ef, -1, SEEK_CUR);
		break;
	}

	if (err) return SW__RECORD_NOT_FOUND;

	written = f_write(capdu->data, 1, capdu->Lc, ef);

	if (written != capdu->Lc)
		return SW__FILE_FILLED;
	else
		return SW__OK;
}

PUBLIC sw_t
cmd_write_record__with_sfi(const CmdAPDU *capdu)
{
	return SW__FUNCTION_NOT_SUPPORTED;
}
