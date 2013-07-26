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
#ifndef _TEST_SUITES_H
#define _TEST_SUITES_H

int build_suite__apdu();
int build_suite__types();
int build_suite__flash();
int build_suite__dev_pflash();
int build_suite__stub_memdev();
int build_suite__smartfs();
int build_suite__somefs();
int build_suite__flxio();
int build_suite__flexcos();
int build_suite__flash_layout();
int build_suite__flash_dev_simple();
int build_suite__tlv_parser();

#endif /* ----- end of macro protection ----- */
