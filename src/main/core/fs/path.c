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

#include <flxlib.h>
#include <i7816.h>

#include <string.h>

#include "path.h"

/**
 * The length of a path is denoted by the number of FID until we reach end of
 * path.
 *
 * @param[in] path a sequence of FID (fid_t)
 *
 * @return Relative length of input path, or EOP (zero) if path is indeed empty
 * or longer than maximum supported path length.
 */
PUBLIC u8
path_length (const path_t p)
{
	u8 i;
	for (i = 0; i < MAX_PATH_DEPTH; i++)
		if (p[i] == EOP) break;

	return i >= MAX_PATH_DEPTH ? EOP : i;
}

/**
 * Identify dirname and basename of a path.
 *
 * @param[in] path, the path to be split up
 * @param[out] dir, the directory part of path
 * @param[out] base, the basename of path
 *
 * @return the length of input path
 *
 * NOTE! we force dir to be preallocated at minimum to length of path!!!
 */
PUBLIC u8
path_split (const path_t path, path_t dir, fid_t *base)
{
	u8 l = path_length(path);

	if (!l) return 0;

	memcpy(DEST(dir), SRC(path), l * sizeof(*path));
	dir[l-1] = EOP;

	*base = path[l-1];
	return l;
}
