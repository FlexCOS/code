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
 *  Provide FlexCOS IO through a FreeRTOS queue handle.
 *
 */
// XXX just for size_t type definition :/
#include <stdlib.h>

#include <const.h>
#include <types.h>
#include <modules.h>

#include <FreeRTOS.h>
#include <queue.h>


PRIVATE xQueueHandle loop_card2term;
PRIVATE xQueueHandle loop_term2card;
PRIVATE bool         is_card_in_read_mode = false;

PRIVATE void
loop_card_start_listen()
{
	is_card_in_read_mode = true;
}

PRIVATE void
loop_card_stop_listen()
{
	is_card_in_read_mode = false;
}

PRIVATE size_t
loop_card_read(u8 *buff, size_t bytes)
{
	size_t n;

	if (!is_card_in_read_mode)
		return 0;


	for (n = 0; n < bytes; n++)
		if (!xQueueReceive(loop_term2card, buff++, 0x96))
			break;

	return n;
}

PRIVATE size_t
loop_card_write(const buff8_t buff, size_t bytes)
{
	size_t n;
	const u8 *c = buff;

	for (n = 0; n < bytes; n++)
		if (!xQueueSend(loop_card2term, c++, 0xff))
			break;

	return n;
}

PUBLIC size_t
loop_term_read(u8 *buff, size_t bytes)
{
	size_t n;

	for (n = 0; n < bytes; n++)
		if (!xQueueReceive(loop_card2term, buff++, 0xFF))
			break;

	return n;
}

PUBLIC size_t
loop_term_write(const u8 *buff, size_t bytes)
{
	size_t n;

	if (!is_card_in_read_mode)
		return 0;

	for (n = 0; n < bytes; n++)
		if (!xQueueSend(loop_term2card, buff++, 0xff))
			break;

	return n;
}

PUBLIC err_t
loop_io()
{
	static const struct module_io _io = {
		.write = loop_card_write,
		.read  = loop_card_read,
		.start_listen = loop_card_start_listen,
		.stop_listen  = loop_card_stop_listen
	};

	loop_term2card = xQueueCreate(0xFF, sizeof(u8));
	loop_card2term = xQueueCreate(0xFF, sizeof(u8));

	return module_hal_io_set(&_io);
}

