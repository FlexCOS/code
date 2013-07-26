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

/*! \file
 *
 */
#pragma once

#ifndef __USE_XIL_TYPES
#define __USE_XIL_TYPES  1
#endif

#include <stddef.h>

/* Although this is not used yet, I'd like to be prepared for switching ...*/
#ifdef __USE_XIL_TYPES
# if   __USE_XIL_TYPES > 0
#   include <xil_types.h>
# else
#   define __USE_STDINT_TYPES
#   include <stdint.h>
# endif
#endif

#ifdef __USE_STDINT_TYPES
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
#endif

#define BUFFER(b)  ((buff_t) (b))

// XXX  Is this bool type implementation too lazy, too error-prone?
// At least we need some kind of strict type checking, locating errors at
// compile time.
typedef u8    bool;
#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	!FALSE
#endif
enum Bool {
	// coding style exception: let 'true' and 'false' appear to be keywords
	true  = TRUE,
	false = FALSE
};

// TODO check usage and remove unused types or move to a more specific location,
// since type declarations in this file are more general.
// XXX actually POSIX reserves any *_t typedef for future use. Remove/rename.
typedef u8*   buff_t;
typedef u8*   buff8_t;
typedef u16*  buff16_t;
typedef u32*  buff32_t;
typedef void* buffv_t;

/* TODO move ISO 7816 related definition to i7816 */
typedef u16     dname_t;   /* smart card 'dentry name' is just a two byte identifier */
typedef u16     fid_t;
typedef u16     sw_t;      /* 16 bit status word type */
typedef fid_t * path_t;

typedef u32  laddr_t;      /* logical address type    */
typedef u32  paddr_t;      /* physical address type   */
typedef u32  baddr_t;      /* block address type      */
typedef u32  raddr_t;      /* relative address type   */

// old Sosse stuff
#define PRG_RDB(x)	(*((u8*)(x)))

struct list_head {
	struct list_head *next, *prev;
};


typedef raddr_t   inode_addr_t; /* inode address is just a synonym of a relative address */
