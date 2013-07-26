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
 * tlv.c
 *
 * Created
 *     Author: Alexander Münn
 */

#include <flxlib.h>
#include <common/list.h>
#include <io/stream.h>

#include "tlv.h"

enum Tlv_BerTag {
	UNI_CLASS = 0x00,
	APP_CLASS = 0x01 << 6,
	CTX_CLASS = 0x10 << 6,
	PRV_CLASS = 0x11 << 6
};

/**
 *  Instead of tlv_nesting structure the parser uses this one, which holds an
 *  additional end marker for each nesting.
 */
struct tlv_parse_scope {
	struct list_head   list;
	u32 tag;
	u16 end_marker;
};

/**
 * Decode SIMPLE-TLV length field.
 *
 * @param[in]  buffer where the length field starts
 * @param[out] decoded length value
 *
 * @return length of tag field in bytes.
 */
PUBLIC u8
tlv_simple_decode_length(const u8 *buff, u16 *l)
{
	u8 byte = 0;
	if (buff[byte] == 0xFF) {
		byte += 1;
		*l = buff[byte] << 8;
		byte += 1;
	}

	*l |= buff[byte];

    return byte + 1;
}

/**
 *  Tag field in SIMPLE-TLV is always one byte long.
 */
static inline u8
tlv_simple_decode_tag(const u8 *buff, u8 *tag)
{
	*tag = buff[0];
	return 1;
}


static inline void
__shift_add(u32 *v, u8 new)
{
	(*v) = ((*v) << 8) | new;
}

/**
 *  Decode a BER tag field from data of an input stream.
 *
 *  Maximum supported tag field length is limited to three bytes.
 *
 *  @param[in]  input stream
 *  @param[out] tag value in raw BER format
 *  @param[out] field size, number bytes that have been taken from stream
 *
 *  @return E_TLV on streaming error
 *          E_TLV_TAG on any kind of bad formated tag field
 *          E_GOOD on success
 */
PRIVATE err_t
__decode_tag_ber(struct stream_in *is, u32 *tag, u8 *fs)
{
#define SUBSEQUENT_BIT BIT_8
	u8 t;

	if (!stream_get(is, &t)) goto end_of_stream;

	*tag = t;
	*fs  = 1;

	if (t == 0x00) return E_TLV_TAG;

	/* bits 1 to 5 all set to one indicate existence of a second byte  */
	if ((t & 0x1F) != 0x1F) goto success;

	/* decode second byte */
	if (!stream_get(is, &t)) goto end_of_stream;
	__shift_add(tag, t);
	(*fs)++;

	if ((t  < 0x1F)
	||  (t == 0x80))
	{
		return E_TLV_TAG;
	}

	if (t < 0x80) goto success;

	/* decode third byte */
	if (!stream_get(is, &t)) goto end_of_stream;
	__shift_add(tag, t);
	(*fs)++;

	if (t & SUBSEQUENT_BIT) return E_TLV_TAG;

success:
	return E_GOOD;
end_of_stream:
	return E_TLV;
}

/**
 *  Decode a BER length field from data of an input stream.
 *
 *  @param[in]  input stream
 *  @param[out] decoded length value
 *  @param[out] field size, number of bytes that have been taken from stream
 *
 *  @return E_GOOD on success
 *          E_TLV  on streaming error
 *          E_TLV_LEN on any length field violations
 */
PRIVATE inline u8
__decode_length_ber(struct stream_in *is, u32 *l, u8 *fs)
{
	u8 i;
	u8 b;
	u8 bytes;

	if (!stream_get(is, &b)) goto end_of_stream;

	if (b < 0x80) {
		*l = b;
		*fs = 1;
		return E_GOOD;
	}

	if (b & 0x78) return E_TLV_LEN;

	bytes = b & 0x07;

	if (!bytes || bytes > 4) return E_TLV_LEN;

	/* XXX instead of this loop we could use stream_read with respect to
	 * endianess */
	for (i = 1, *l = 0; i <= bytes; i++) {
		if (!stream_get(is, &b)) goto end_of_stream;
		*l |= b << ((bytes - i) * 8);
	}
	*fs = bytes + 1;

	return E_GOOD;

end_of_stream:
	return E_TLV;
}

PRIVATE inline struct tlv_parse_scope *
scope_new(void)
{
	return malloc(sizeof(struct tlv_parse_scope));
}

PRIVATE inline void
scope_destroy(struct tlv_parse_scope *s)
{
	free(s);
}

PRIVATE u8
__pop_nestings(u32 pos, struct list_head *nesting)
{
	struct tlv_parse_scope *scope;

	loop {
		if (list_empty(nesting))
			return E_GOOD;

		scope = list_first_entry(nesting, struct tlv_parse_scope, list);

		if (pos < scope->end_marker)
			return E_GOOD;
		/* Overflow of parent TLV object? */
		if (pos > scope->end_marker)
			return E_TLV_FIT;

		list_del(&(scope->list));
		scope_destroy(scope);
	}
}

PRIVATE err_t
__push_nesting(struct tlv_parse_ctx *tlv, u32 pos)
{
	struct tlv_parse_scope *scope;

	scope = scope_new();
	if (!scope) return E_ALLOC;

	scope->tag        = tlv->tag;
	scope->end_marker = tlv->length + pos;

	list_add(&scope->list, &tlv->nesting);

	return E_GOOD;
}

PRIVATE void
__clean_nestings(struct list_head *nesting)
{
	struct tlv_parse_scope *s;
	struct list_head *pos, *q;
	list_for_each_safe(pos, q, nesting) {
		s = list_entry(pos, struct tlv_parse_scope, list);
		list_del(pos);
		scope_destroy(s);
	}
}

PRIVATE inline bool
__is_constructed(u32 tag) {
	tag = tag & (0xFF << 16) ? tag & (0x20 << 16) :
	      tag & (0xFF <<  8) ? tag & (0x20 <<  8) :
	      tag &  0x20;

	return tag != 0;
}

/**
 *  A non-recursive BER-TLV parser.
 *
 *
 */
PUBLIC err_t
tlv_parse_ber(const u8 *src, const u16 length, fp_tlv_visit visit, void *opaque)
{
	err_t  err;
	u8     field_size;
	enum   Tlv_Parse_Cmd   cmd;
	struct array_stream_in ais = ARRAY_STREAM_IN(src, length);
	struct tlv_parse_ctx   tlv;
	struct stream_in       *is = &ais.impl;

	u16 pos = 0;

	INIT_LIST_HEAD(&tlv.nesting);

	while (pos < length) {
decode_next:
		err = __decode_tag_ber(is, &tlv.tag, &field_size);
		if (err) goto cleanup;
		pos += field_size;

		err = __decode_length_ber(is, &tlv.length, &field_size);
		if (err) goto cleanup;
		pos += field_size;

		tlv.value = src + pos;

		cmd = visit(&tlv, opaque);

		if (cmd == STOP) {
			err = E_TLV;
			goto cleanup;
		}

		/**
		 *
		 */
		if ((__is_constructed(tlv.tag))
		&&  (cmd == STEP_INTO))
		{
			if ((err = __push_nesting(&tlv, pos)))
				goto cleanup;

			goto decode_next;
		}

		if (stream_skip(is, tlv.length) != tlv.length) {
			err = E_TLV_VAL;
			goto cleanup;
		}

		pos += tlv.length;

		if ((err = __pop_nestings(pos, &tlv.nesting)))
			break;
	}

	err = E_GOOD;
cleanup:
	__clean_nestings(&tlv.nesting);
	return err;
}

