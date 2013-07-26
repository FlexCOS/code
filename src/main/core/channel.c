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
 * channel.c
 *
 * Created
 *    Author: alexander Münn
 */

#include <flxlib.h>
#include <io/stream.h>
#include <common/list.h>
#include <mm/pstore.h>

#include "channel.h"

struct channel_intern {
	struct channel pub;
	struct pstore  session;
	struct pstore  request;
};

/* TODO there is no channel management right now */
PRIVATE struct channel_intern _chan = {{0}};

struct object {
	struct list_head list;
	u32  key;
	void *obj;
	void (*destroy)(void);
};

PUBLIC err_t
channel_setup(void)
{
	pstore_init(&_chan.session);
	pstore_init(&_chan.request);

	return E_GOOD;
}

PUBLIC struct channel *
__current(void)
{
	return &_chan.pub;
}

PUBLIC struct pstore *
__session(void)
{
	return &_chan.session;
}

PUBLIC struct pstore *
__request(void)
{
	return &_chan.request;
}
