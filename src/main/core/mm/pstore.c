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
#include <common/list.h>

#include "pstore.h"

struct pstore_entry {
	struct list_head entry;
	u32 key;
	void *obj;
	fp_free dealloc;
};

PRIVATE struct pstore_entry *
__pstore_find(const struct pstore *ps, u32 key)
{
	struct pstore_entry *e;
	list_for_each_entry(e, &ps->list, entry) {
		if (e->key == key) return e;
	}
	return NULL;
}

/**
 *  XXX ambigious?
 */
PRIVATE void
pentry_release(struct pstore_entry *e)
{
	e->dealloc(e);
}

PRIVATE inline struct pstore_entry *
pentry_alloc(void)
{
	return malloc(sizeof(struct pstore_entry));
}

PRIVATE inline void
pentry_free(struct pstore_entry *e)
{
	free(e);
}

PRIVATE inline void
pstore_attach(struct pstore *ps, struct pstore_entry *e)
{
	list_add(&e->entry, &ps->list);
}

PRIVATE inline void
pstore_detach(struct pstore *ps, struct pstore_entry *e)
{
	PARAM_UNUSED(ps);

	list_del(&e->entry);
}

PUBLIC void
pstore_init(struct pstore *ps)
{
	INIT_LIST_HEAD(&ps->list);
}

PUBLIC void *
pstore_alloc(struct pstore *ps, u32 key, size_t bytes)
{
	struct pstore_entry *entry;

	// XXX errno E_INVAL
	if (!ps) return NULL;

	entry = __pstore_find(ps, key);

	if (entry) {
		pentry_release(entry);
	} else {
		entry = pentry_alloc();
	}

	if (!entry) return NULL; // XXX errno E_NOMEM

	entry->obj = malloc(bytes);
	entry->dealloc = free;

	if (!entry->obj) {
		pentry_free(entry);
		// XXX errno E_NOMEM
		return NULL;
	}

	pstore_attach(ps, entry);

	return entry->obj;
}

PUBLIC void *
pstore_lookup(struct pstore *ps, u32 key)
{
	struct pstore_entry *e;

	e = __pstore_find(ps, key);

	return e ? e->obj : NULL;
}

PUBLIC err_t
pstore_add(struct pstore *ps, u32 key, void *ptr, fp_free free_func)
{
	struct pstore_entry *entry;

	if (__pstore_find(ps, key))
		return E_EXIST;

	entry = pentry_alloc();

	if (!entry)
		return E_NOMEM;

	entry->obj = ptr;
	entry->dealloc = free_func;

	pstore_attach(ps, entry);

	return E_GOOD;
}
