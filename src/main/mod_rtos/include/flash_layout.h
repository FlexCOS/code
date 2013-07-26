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

#define FLASH_CLUSTER(off, s, p, ps)  { \
	.absolute_offset = (off),       \
	.sectors = (s),                 \
	.pages = (p),                   \
	.page_size = (ps)               \
}
#define FLASH_CLUSTER_UNUSED_SLOT   {0}

struct flash_cluster {
	/* XXX store size or calculate on demand? */
	/* absolut start address of this cluster */
	u32 absolute_offset;
	/* number of sectors this cluster contains */
	u16 sectors;
	/* number of pages each sector contains */
	u16 pages;
	/* number of bytes each page contains */
	u16 page_size;
};

struct flash_layout {
	u32 total_size;
	u8  clusters;
	/**
	 * A flash device may contain areas of different sector sizes. Consider
	 * a group of sequential sectors of same size as a cluster.
	 *
	 * Here we expect each sector to be the smallest erase-region, whereas
	 * write access is bound to pages, i.e. any write command send to a
	 * Flash Controller must not address memory across pages.
	 *
	 * NOTE: the absolute_offset of successor cluster implicates size of
	 * each cluster itself.
	 */
	/* FIXME Magic Number */
	struct flash_cluster cluster[4];
};

/**
 * A Flash Location puts an absolute address into its layout context.
 */
struct flash_loc {
	u32 abs; /* original absolute address */
	u8  c;   /* cluster id */
	u16 s;   /* sector id */
	u16 p;   /* page id */
	u16 o;   /* offset within page */
	const struct flash_layout *ly;
};

static inline bool
flash_layout__contains(const struct flash_layout *ly, u32 addr)
{
	return (addr < ly->total_size);
}

err_t flash_layout__locate(const struct flash_layout *, u32 abs, struct flash_loc *);
err_t flash_layout__append_cluster(struct flash_layout *, u16, u16, u16);

/**
 *  Update the absolute address of a location according to values of cluster,
 *  sector, page and offset.
 */
err_t flash_loc__relocate(struct flash_loc *);
err_t flash_loc__inc_page(struct flash_loc *);
err_t flash_loc__inc_sector(struct flash_loc *);

static inline const struct flash_cluster *
__cluster(const struct flash_loc *loc)
{
	return &loc->ly->cluster[loc->c];
}

static inline u32
__page_size(const struct flash_cluster *c) {
	return c->page_size;
}

static inline u32
__sector_size(const struct flash_cluster *c) {
	return c->pages * c->page_size;
}

static inline u32
__cluster_size(const struct flash_cluster *c) {
	return c->sectors * __sector_size(c);
}

/**
 *  @return Size of page the location belongs to.
 */
static inline u16
flash_loc__page_size(const struct flash_loc *loc)
{
	return __page_size(__cluster(loc));
}

/**
 *  @return size of sector the location belongs to.
 */
static inline u32
flash_loc__sector_size(const struct flash_loc *loc)
{
	return __sector_size(__cluster(loc));
}
/**
 *  @return offset from start of sector the location belongs to.
 */
static inline u32
flash_loc__sector_addr(const struct flash_loc *loc)
{
	return flash_loc__page_size(loc) * loc->p + loc->o;
}


/**
 *
 */
static inline err_t
flash_loc__sector_start(const struct flash_loc *ref, struct flash_loc *start)
{
	*start = *ref;
	start->p = 0;
	start->o = 0;
	return flash_loc__relocate(start);
}

/**
 *
 */
static inline err_t
flash_loc__sector_end(const struct flash_loc *ref, struct flash_loc *end)
{
	*end = *ref;
	end->p = __cluster(ref)->pages - 1;
	end->o = __cluster(ref)->page_size - 1;
	return flash_loc__relocate(end);
}

/**
 *
 */
static inline err_t
flash_loc__sector_next(const struct flash_loc *ref, struct flash_loc *next)
{
	*next = *ref;
	return flash_loc__inc_sector(next);
}

static inline err_t
flash_loc__page_end(const struct flash_loc *ref, struct flash_loc *end)
{
	*end = *ref;
	end->o = flash_loc__page_size(ref) - 1;
	end->abs = ref->abs - ref->o + end->o;
	return E_GOOD;
}

err_t flash_loc__next_sec_end(const struct flash_loc *, struct flash_loc *);

typedef err_t (*fp_do_blkwise)(const struct flash_loc *, const struct flash_loc *, void *);

err_t flash_layout__do_sector_wise(const struct flash_layout *, u32, u32, void *, fp_do_blkwise);
err_t flash_layout__do_page_wise(const struct flash_layout *, u32, u32, void *, fp_do_blkwise);

