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

#include <ruby.h>

#include <const.h>
#include <types.h>
#include <modules.h>
#include <array.h>
#include <local_io.h>
#include <flexcos.h>

MODULES(local_io);

VALUE
flx_send(VALUE clazz, VALUE arr)
{
	VALUE *v;
	u8 buff[256];
	struct array capdu = CArray(buff);
	u16 received;
	u16 send;
	u8  *c;
	u32 byte;

	switch (TYPE(arr)) {
	case T_ARRAY: break;
	default:
		rb_raise(rb_eTypeError, "Byte array expected.");
		return;
	}

	for (v = RARRAY_PTR(arr); v < RARRAY_PTR(arr) + RARRAY_LEN(arr); v++) {

		byte = NUM2INT(*v);
		if (byte > 0xFF)
			rb_warn("One element does not fit into byte");

		if (array_put(&capdu, (u8) byte)) continue;
		/* Could not put byte into array:
		 * send partial command, free array and try to re-put input
		 * value. */
		local_capdu_send(capdu.val, capdu.length);
		array_truncate(&capdu);

		if (!array_put(&capdu, NUM2INT(*v)))
			// unlikely...
			rb_raise(rb_eRuntimeError, "FlexCOS-extension: weired o.O");
	}
	local_capdu_send(capdu.val, capdu.length);

	flexcos_one_shot();

	VALUE response = rb_ary_new();
	do {
		received = local_rapdu_recv(buff, sizeof(buff));
		for_each(c, buff, received) {
			rb_ary_push(response, INT2FIX(*c));
		}
	} while (received == sizeof(buff));

	return response;
}

void
Init_FlexCOS(void)
{
	if (modules_init())
		rb_raise(rb_eRuntimeError, "Failed to init FlexCOS");

	VALUE cFlexCOS = rb_define_class("FlexCOS", rb_cObject);

	rb_define_singleton_method(cFlexCOS, "send", flx_send, 1);

	return;
}
