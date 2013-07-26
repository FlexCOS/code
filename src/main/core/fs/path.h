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
#ifndef _PATH_H_
#define _PATH_H_

#define MAX_PATH_DEPTH 4

u8 fid_is_allowed(fid_t fid);


err_t path_to_absolute( const path_t, path_t );
err_t path_to_relative( const path_t, path_t );

u8    path_length(const path_t);

/**
 * Copy and terminate the parent directory part from input path to an output
 * path. Additionally set basename and return length of path p, i.e., the relative
 * subdirectory level.
 */
u8    path_split(const path_t, path_t, fid_t *);


/**
 * Provide a function moving the path pointer along its FIDs.
 *
 * @return EOP if end of FID sequence has been reached
 */
static inline fid_t
phead( const path_t p )
{
	return p[0];
}

static inline path_t
ptail( const path_t p )
{
	return phead(p) == EOP ? 0 : p + 1;
}
/**
 * @param[in,out] pointer to a file identifier sequence
 * @return current file identifier or EOP
 */
static inline fid_t
pwalk( path_t *_p ) {
	fid_t f;
	if ((f = phead(*_p)) != EOP) {
 		*_p = ptail(*_p);
	}
	return f;
}

static inline u8
path_is_absolute(const path_t p)
{
	return phead(p) == MF;
}

#endif
