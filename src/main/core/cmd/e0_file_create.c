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
#include <common/list.h>
#include <i7816.h>
#include <flxio.h>
#include <apdu.h>
#include <apdu/commands.h>
#include <channel.h>
#include <tlv.h>


PUBLIC sw_t
cmd_file_create__with_sfi(const CmdAPDU *capdu)
{
	/* P1 ~ file descriptor byte
	 * P2 ~ read short identifier (bits 8 to 4)
	 */
	return SW__FUNCTION_NOT_SUPPORTED;
}

/**
 *  File Descriptor Byte plus optional fields.
 *
 *  L=1   => FDB only
 *  L=2   => FDB + Data Coding Byte
 *  L=3/4 => FDB + DCB + max record size on one or two bytes
 *  L=5/6 => FDB + DCM + 2byte RecSize plus one or two bytes RecCount
 *
 *  FIXME if else if else if else o.O
 */
PRIVATE enum Tlv_Parse_Cmd
__fcp_parse__fd_plus(const u8 *data, u8 length, struct i7_fcp *fcp)
{
	if (!length || length > 6) return STOP;

	fcp->fdb = data[0];
	if (length > 1)
		fcp->dcb = data[1];
	else
		return NEXT;

	if (length > 2)
		fcp->rsize = data[2];
	else
		return NEXT;

	if (length > 3) {
		fcp->rsize <<= 8;
		fcp->rsize  |= data[3];
	}
	else
		return NEXT;

	if (length > 4)
		fcp->rcount = data[4];
	else
		return NEXT;

	if (length > 5) {
		fcp->rcount <<= 8;
		fcp->rcount  |= data[5];
	}

	return NEXT;
}

PRIVATE inline enum Tlv_Parse_Cmd
__fcp_parse__fid(const u8 *data, u8 length, struct i7_fcp *fcp)
{
	if (length != 2 || fcp->fid) return STOP;

	fcp->fid  = data[0] << 8;
	fcp->fid |= data[1];

	return NEXT;
}

PRIVATE inline enum Tlv_Parse_Cmd
__fcp_parse__life_cycle_status(const u8 *data, u8 length, struct i7_fcp *fcp)
{
	if (length != 1 || fcp->lcs) return STOP;

	fcp->lcs = data[0];

	return NEXT;
}

PRIVATE enum Tlv_Parse_Cmd
accept_fcp_only(const struct tlv_parse_ctx *tlv, void *opaque)
{
	struct i7_fcp      *fcp = opaque;

	/*
	 * FCP-Tag is mandatory be top level tag.
	 */
	if (list_empty(&tlv->nesting)) {
		if (tlv->tag == 0x62)
			return STEP_INTO;
		else
			return STOP;
	}

	switch (tlv->tag) {
	/*
	 * File IDentifier
	 */
	case FCP_FID:
		return __fcp_parse__fid(tlv->value, tlv->length, fcp);
	/*
	 * Life Cycle Status Byte
	 */
	case FCP_LCS:
		return __fcp_parse__life_cycle_status(tlv->value, tlv->length, fcp);
	/*
	 * File Descriptor Byte plus optional fields
	 */
	case FCP_FD_PLUS:
		return __fcp_parse__fd_plus(tlv->value, tlv->length, fcp);
	/*
	 * Any other fields are not supported yet
	 */
	default:
		return STOP;
	}
}

/**
 *  This Command APDU handler should be called, if P1 and P2 have been set to
 *  zero.
 */
PUBLIC sw_t
cmd_file_create__from_fcp(const CmdAPDU *capdu)
{
	FILE *fh;
	struct i7_fcp fcp = {0};
	extern Channel *current;

	err_t err;

	if (current->ef) f_close(current->ef);

	if (!capdu->Lc) return SW__WRONG_LENGTH;

	err = tlv_parse_ber(capdu->data, capdu->Lc, accept_fcp_only, &fcp);

	if (err) return SW__WRONG_FCI;

	fh = f_create(&fcp);

	if (!fh) return 0x6300;

	current->ef = fh;

	return SW__OK;
}
