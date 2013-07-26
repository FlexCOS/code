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

#define Array(buff, s)          { .val = (buff), .max = (s), .length = 0 }
#define CArray(buff)            Array(buff, sizeof((buff)))

/**
 *
 */
struct array {
	union {
		u16   length;           /* current used length */
		u16   len;
	};
	union {
		u16 __max;
		u16 const max;
		u16 const size __deprecated;
	};
	union {
		u8    *const val;       /* provide a more readable(?) alias */
		u8        *__val;
		u8 const *const __v __deprecated;
		u8       *const v __deprecated;         /* TODO refactor: remove */
	};
};

struct array_iterator {
	const struct array *const arr;
	u8           pos;
};

typedef struct array Array;
typedef struct array array;

/**
 *  Iterate over each element of an array:
 *
 *  array_each(arr, c) { *c += 2 }
 */
#define array_each(arr, c) \
	for ((c) = (arr)->val; \
	     (c) < (arr)->val + (arr)->len; \
	     (c) += 1)

/* Do we need in place mapping? Either */
static inline err_t
array_map(struct array *arr, u8 (*map)(u8))
{
	u8 *c;
	array_each(arr, c) *c = map(*c);

	return E_GOOD;
}
/* or */
//#define array_map(arr, map)                         \
//	u8 *__c; \
//	for (__c = (arr)->val;              \
//                 __c < (arr)->val + (arr)->len; \
//                 __c +=1) {                     \
//		*__c = map(*__c);       \
//	}


PUBLIC u16  array_append(array *, const u8 *, u16);
PUBLIC u8   array_put(array *, u8);
PUBLIC u16  array_fill(array *, u8);
PUBLIC void array_clean(array *);
PUBLIC void array_copy(const struct array *, struct array *);
PUBLIC u8   array_shift(array *);
PUBLIC u16  array_shift_out(array *, u16);

PUBLIC void          array_free(struct array *);
PUBLIC struct array *array_alloc(u16);

static inline void
array_init(array *arr, u8 *buff, u16 size)
{
	arr->__val = buff;
	arr->__max = size;
	arr->len   = 0;
}

/**
 *  Provide an alias for appending one element to the end of an array.
 *
 *  @return One on success, i.e. element count has been increased by one.
 */
static inline u8
array_push(array *arr, u8 v)
{
	return array_put(arr, v);
}

/**
 *  @return Number of unused bytes.
 */
static inline u16
array_bytes_left(Array *arr)
{
	return arr->max - arr->length;
}

/**
 *  TODO In most use cases this method should be replaceable by something like:
 *  u8 *array_reserve(arr, bytes)
 */
static inline u8 *
array_end(Array *arr)
{
	/* XXX hm, returning NULL is not safe in cases like
	 * memset(array_end(arr), 0x00, array_bytes_left(arr));
	 */
	return array_bytes_left(arr) ? arr->val + arr->length : NULL;
}

static inline u8
array_last(array *arr)
{
	/* FIXME What about arr->len == 0 ...
	 * Should we return 0 and set errno, or is the caller responsible for
	 * precondition checks?
	 */
	return arr->val[arr->len-1];
}

/**
 *  TODO Choose a better (self explaining) alias for setting array length to
 *  zero. I'm not sure if 'reset' is less misleading then 'truncate'.
 *  Additionally it needs to be distinguishable from cleaning/clearing complete
 *  memory area in a way, that one does not suspect array_reset(key) purging key
 *  data.
 */
static inline void __deprecated_msg("Use 'array_reset()' instead.")
array_truncate(Array *arr)
{
	arr->length = 0;
}

static inline void
array_reset(Array *arr)
{
	arr->length = 0;
}

static inline bool
array_iter_has_next(struct array_iterator *iter)
{
	return iter->pos < iter->arr->length;
}

/**
 *  @return Value of next element, default to zero if the end has been reached.
 *
 *  @NOTE Unless your array does not contain any zeros, this method won't let
 *  you distinguish between any zero element or end of sequence. Therefore you 
 *  might ask the iterator if there is a next element as usual.
 */
static inline u8
array_iter_next(struct array_iterator *iter)
{
	return array_iter_has_next(iter) ? iter->arr->val[iter->pos++] : 0;
}

