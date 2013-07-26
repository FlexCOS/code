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

#pragma once

/**
 *  This method will register a io module into FlexCOS.
 *
 *  This module ...
 */
err_t loop_io();

/**
 *  Emulate terminal access to read data sent by SmartCard.
 *
 *  @return Number of read bytes.
 */
size_t loop_term_read(buff8_t, size_t);

/**
 *  Emulate terminal access to send data to SmartCard OS.
 *
 *  @return Number of bytes actually sent.
 */
size_t loop_term_write(const buff8_t, size_t);
