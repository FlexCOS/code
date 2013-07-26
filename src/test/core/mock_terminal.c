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

#include <stdlib.h>
#include <const.h>
#include <types.h>

#include <pipe_io.h>

PRIVATE sw_t last_sw;

/**
 *  
 */
PUBLIC size_t
mock_terminal__send(const buff8_t buff, size_t s)
{
	return pipe_term_write(buff, s);
}

/** 
 *
 */
PUBLIC size_t
mock_terminal__recv(buff8_t buff, size_t s)
{
	size_t recv = pipe_term_read(buff, s);

	last_sw = 0x0000;
	if (recv >= 2) {
		last_sw |= buff[recv-2] << 8;
		last_sw |= buff[recv-1];
	}

	return recv;
}

PUBLIC sw_t
mock_terminal__last_sw()
{
	return last_sw;
}
