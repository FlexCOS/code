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
#ifndef _SOMEFS_H_
#define _SOMEFS_H_

struct mem_dev;
struct fs_mount;

enum SOMEFS_CONSTANTNS {
	SOMEFS_MAGIC = 0x34,
	NO_ADDR = 0x00
};


err_t somefs_mkfs( struct mem_dev * );
err_t somefs_mount( struct mem_dev *, struct fs_mount * );


#endif
/* ----- end of macro protection _SOMEFS_H_ */
