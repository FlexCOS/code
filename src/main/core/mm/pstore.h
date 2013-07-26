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
 *  PStore -- some kind of pointer directory service store foo bla...
 */

struct pstore {
	struct list_head list;
};

typedef void (*fp_free)(void *);

void  pstore_init(struct pstore *);
void *pstore_alloc(struct pstore *, u32, size_t);
void  pstore_free(struct pstore *, u32);
err_t pstore_add(struct pstore *, u32, void *, fp_free);
void *pstore_lookup(struct pstore *, u32);
void  pstore_release(struct pstore *, u32);
void  pstore_free_all(struct pstore *);
