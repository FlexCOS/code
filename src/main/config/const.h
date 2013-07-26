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

/*
 * const.h
 *
 *  Created on: Jul 17, 2012
 *      Author: Alexander Münn
 */
#pragma once

#ifndef _CONST_H_
#define _CONST_H_

/*--------------------------------*/
/* Global common constants        */
/*--------------------------------*/
#define PUBLIC
#define PRIVATE	static
/*! Variable is in code space. Be carfull with pointers to this space. */
#define	CODE

#define __BUFF(b)  ((void *) b)
#define DEST(buff) __BUFF(buff)
#define SRC(buff)  __BUFF(buff)

#ifndef EOF
#define EOF 0
#endif

#define BIT_1     (1 << 0)
#define BIT_2     (1 << 1)
#define BIT_3     (1 << 2)
#define BIT_4     (1 << 3)
#define BIT_5     (1 << 4)
#define BIT_6     (1 << 5)
#define BIT_7     (1 << 6)
#define BIT_8     (1 << 7)

#define ERR_CLASS_MASK   (0xFF << 24)
#define ERR_CODE(err)    ((err) & ~ERR_CLASS_MASK)
#define ERR_CLASS(err)   ((err) &  ERR_CLASS_MASK)


typedef int err_t;

enum Err_Class {
	/* MSB is reserved */
	E_SYSTEM    = 0x40 << 24,
	E_INTERN    = 0x20 << 24,
	E_BAD_PARAM = 0x10 << 24      /**< catch error early. Flag range or null pointer errors as cause of invalid parameter */
};

/*--------------------------------*/
/* Basic error codes              */
/*--------------------------------*/
enum Err_Code {
	SUCCESS     = 0,
	E_GOOD      = SUCCESS,
	E_FAILURE   = 0x0FFFFFFF,
	/** an additional alias for writing (call() IS E_FAILED)
	 * instead of (call() IS E_FAILURE) or (call() IS !E_GOOD) */
	E_FAILED    = E_FAILURE,
	E_NO_LOGIC  = 0x07FFFFFF,
	/** often functions depend on system resources. */
	/* common error codes */
	E_NULL_PTR  = 2,
	E_ADDRESS   = 3,               /**< invalid memory address */
	E_RANGE     = E_ADDRESS,
	E_BUSY      = 4,
	E_MEMORY    = 5,
	E_ALLOC     = 6,
	E_NOMEM     = E_ALLOC,
	E_NOENT     = 7,                 /** no entry */
	E_BADF      = 8,                 /** Bad file number */
	E_EXIST     = 9,
	/* file system specific error codes */
	E_FS        = 32,
	E_FS_INO    = 33,
	E_FS_EXIST  = 34,
	E_BADFD     = 35,                /**< an invalid file descriptor */
	E_FS_BUSY   = 36, 
	/* TLV encoding errors */
	E_TLV       = 64,                /**< TLV inconsistency. */
	E_TLV_TAG   = 65,                /**< bad tag field encoding */
	E_TLV_LEN   = 66,                /**< bad length field encoding */
	E_TLV_VAL   = 67,                /**< value does not match to length */
	E_TLV_FIT   = 68,                /**< TLV object out of bounds */
	E_TLV_DEPTH = 69,                /**< maximum TLV nesting is bounded by system caps*/
	/* hardware specific error codes */
	E_HW        = 100,               /**< general hardware error */
	E_HWW       = 101,               /**< hardware error on write */
	E_HWR       = 102,               /**< hardware error on read */
	E_HWBUSY    = 103,               /**< hardware error on busy device */
	E_APDU_TOO_SHORT = 200,
	E_APDU_FORMAT = 201,
	E_APDU_LENGTH = 202,
	E_APDU_LC     = 203,
	E_APDU_LE     = 204
};

/*
 * FIXME Put this anywhere else, e.g. into errno.h or likewise...
 */
static inline enum Err_Class
eclass(err_t err)
{
	return ERR_CLASS(err);
}
static inline enum Err_Code
ecode(err_t err)
{
	return ERR_CODE(err);
}


#define CHECK_PARAM__NOT_ZERO(x) if ((x) == 0) return E_BAD_PARAM
#define CHECK_PARAM__NOT_NULL(x) if ((x) == NULL) return E_BAD_PARAM | E_NULL_PTR
#define PARAM_UNUSED(x) ((void) (x))

/* TODO Export to a compiler dedicated header file. */
#define __deprecated __attribute__((deprecated))
#define __packed     __attribute__((packed))
#define __hot        __attribute__((hot))
#define __warn(S)    __attribute__((warning(#S)))
#define WARN(S) __warn(S)
#define __TODO (msg) warning (msg)

#define __deprecated_msg(msg) __attribute__((deprecated (msg)))


#define halt() while(1) {}
#define loop   for ( ;; )
#define for_each(c, arr, max)  for ((c) = (arr); (c) < ((arr) + (max)); (c)++)

#define MIN( a, b )  ( (a) < (b) ? (a) : (b) )
#define MAX( a, b )  ( (a) < (b) ? (b) : (a) )
#define DIST( a, b ) ( (a) < (b) ? ((b) - (a)) : ((a) - (b)) )

#define abs( v )     ( (v) <  0  ? -(v) : (v))
/* Get length of a standard C array:   type_t foo[] */
#define LENGTH(arr)  ( sizeof((arr)) / sizeof(*(arr)) )

/*--------------------------------*/
/* Application specific constants */
/*--------------------------------*/

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
        const typeof(((type *)0)->member) * __mptr = (ptr);     \
        (type *)((char *)__mptr - offsetof(type, member)); })
#endif


#define SOMEFS_MAX_DIR_ENTRIES     16

/**
 * Configure how much struct objects of each type should be
 * preallocated in programm memory.
 */
#define FS_MAX_ACTIVE_INODES       8
#define FS_MAX_ACTIVE_DENTRIES     8
#define FS_MAX_ACTIVE_FILES        8

#endif /* _CONST_H_ */
