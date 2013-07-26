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
#include <array.h>
#include <apdu.h>
#include <common/list.h>
#include <channel.h>
#include <mm/pstore.h>
#include <io/stream.h>
#include <tlv.h>

#include <miracl.h>
#include <ecc.h>
#include <cryptools.h>

enum EC2PTlvFoo {
	EC2P_MSG = 0x01,
	EC2P_PIN = 0x02
};

struct ec2p_request {
	struct array *msg;
	struct array *pin;
};

/* I think think tlv parsing is a good reason to introduce exceptions. */
PRIVATE enum Tlv_Parse_Cmd
require_msg_and_pin(const struct tlv_parse_ctx *tlv, void *opaque)
{
	struct array        *tlv_data;
	struct ec2p_request *req = opaque;
	enum ChannelObjectID id;

	if (list_empty(&tlv->nesting)) {
		if (tlv->tag == 0x21)
			return STEP_INTO;
		else
			return STOP;
	}

	tlv_data = array_alloc(tlv->length);
	if (!tlv_data) return STOP;

	switch (tlv->tag) {
	case EC2P_MSG:
		id = OID_EC2P_MSG;
		req->msg = tlv_data;
		break;
	case EC2P_PIN:
		id = OID_EC2P_PIN;
		req->pin = tlv_data;
		break;
	default:
		array_free(tlv_data);
		return STOP;
	}

	array_append(tlv_data, tlv->value, tlv->length);

	/* Adding fails if either there is already an object of 'id' or pstore 
	 * fails to allocate memory for internal structure. */
	if (pstore_add(chan_session, id, tlv_data, (fp_free) array_free)) {
		array_free(tlv_data);
		return STOP;
	}

	return NEXT;
}

PUBLIC sw_t
cmd_ec2ps_start(const CmdAPDU *capdu)
{
	struct ec2p_request req;
	struct ecc_dom *dom;
	struct ecc_pk  *pk_srv;

	struct stream_out *hmac_stream;
	struct stream_out *ecies_stream;
	u8     __key[0] = {};
	u8     *c;
	struct array      key = CArray(__key);

	err_t err;

	err = tlv_parse_ber(capdu->data, capdu->Lc, require_msg_and_pin, &req);
	if (err)
		return SW__LC_TLV_CONFLICT;

	/* Message and PIN are mandatory for this command */
	if ((req.pin == NULL)
	||  (req.msg == NULL))
	{
		return SW__WRONG_DATA;
	}

	hmac_stream = hmac_stream_out_create(&key, current->response);

	array_each(req.msg, c) stream_put(hmac_stream, *c);

	stream_close(hmac_stream);

	// load EC-Dom from default file
	// load app file 
	// load PK_srv
	// init ECIES encryption stream
	// init HMAC stream
	
	// stream_write(ecies, message)
	// stream_write(ecies, hash(pin))
	// session_push(message)
	// session_push(pin)
	// tlv end
	
	// handle error?!
	// ecc_gen(dom, R, k)
	// session_push(R)
	// session_push(k)
	// write(R)
	
	// generate random rho
	// session_push(rho)
	// write(rho)
	
	// close(ECIES)
	// filestream(tau)
	// stream_transfer(HMAC, tau)
	
	// stream_close(HMAC)
	// return
	return SW_NOT_SUPPORTED;
}
