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

struct stream_out;

struct list_head;

struct tlv_object;
struct tlv_ber_ctx;
struct tlv_parse_ctx;

typedef struct tlv_object TlvObject;

/** 
 *  During TLV parsing callback method can control behaviour.
 */
enum Tlv_Parse_Cmd {
	STEP_INTO = 0x01,
	NEXT      = 0x02,
	STOP      = 0x03
};
/**
 *  General callback for parsing TLV data.
 */
typedef enum Tlv_Parse_Cmd (*fp_tlv_visit)(const struct tlv_parse_ctx *, void *);

/**
 *  @return number of written bytes
 */
PUBLIC u32
tlv_encode_ber(TlvObject *, void *_ctx, struct stream_out *);

/**
 * Create a new BER-TLV object of primitive encoded data.
 */
PUBLIC TlvObject *
tlv_create_ber_prim(u8 _tag, u32 _length, const u8 *value);

PUBLIC err_t
tlv_parse_ber(const u8 *, u16, fp_tlv_visit, void *);

/**
 * Create a BER-TLV object with constructed
 * encoding of the data part.
 */
PUBLIC TlvObject *
tlv_create_ber_cons(u8 _tag, TlvObject *);

PUBLIC TlvObject *
tlv_object_join(TlvObject *, TlvObject *);

/**
 * Crawl a TlvObject tree for a TlvObject with a specific tag. Start any where
 * in the tree at object '_start'. We differ between 'depth-first' and
 * 'breadth-first' search algorithm.
 */
PUBLIC TlvObject *
tlv_find_next(u8 _tag, TlvObject *_start, u8 _method);

PUBLIC void
tlv_release_all(TlvObject *);

#define BER_TLV_CLS_BYTES = 0x03 << 6
#define BER_TLV_ENC_BYTES = 0x01 << 5

enum BER_TLV_CLASSES {
	UNI = 0x00 << 6,
	APP = 0x01 << 6,
	CTX = 0x02 << 6,
	PRV = 0x03 << 6
};

struct tlv_nesting {
	struct list_head list;
	u32 tag;
};

struct tlv_parse_ctx {
	struct list_head   nesting;
	u32                tag;
	u32                length;
	const u8           *value;
};


struct tlv_object {
	u32               enc_bytes; /* real tag-length */
	u8                tag;
	u32               length;

	/* a primitive BEL-TLV object has *value set to a valid memory
	 * address */
	const u8          *value;
	/* a constructed BER-TLV object points instead to its first child */
	struct tlv_object *first_child;

	struct tlv_object *parent;
	struct tlv_object *next;        /* each TlvObject is part of a singly linked list */
};


