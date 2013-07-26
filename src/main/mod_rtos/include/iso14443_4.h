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

/* -------------------------------------------------------------------------- */
/* PCB - Protocol Control Byte */

/* First two MSB defining PCB block type */
#define ISO14443_4_MASK_BLOCK              0x90
#define ISO14443_4_I_BLOCK                 0x00
#define ISO14443_4_R_BLOCK                 0x80
#define ISO14443_4_S_BLOCK                 0x90

/* Each block type has mandatory bits. Set either to one or zero. */
#define ISO14443_4_R_BLOCK_MASK            0xE6 // (11100110)b
#define ISO14443_4_R_BLOCK_BITS            0xA2 // (10100010)b

#define ISO14443_4_S_BLOCK_MASK            0xC7 // (11000111)b
#define ISO14443_4_S_BLOCK_BITS            0xC2 // (11000010)b

#define ISO14443_4_I_BLOCK_MASK            0xE2 // (11100010)b
#define ISO14443_4_I_BlOCK_BITS            0x02 // (00000010)b


#define ISO14443_4_R_ACK                   0xA2
#define ISO14443_4_R_NAK                   0xB2

#define ISO14443_4_PCB_CID_BIT             0x08
#define ISO14443_4_PCB_NAD_BIT             0x04
#define ISO14443_4_PCB_NAK                 0x10
#define ISO14443_4_PCB_CHAINING            0x10
#define ISO14443_4_BLOCK_NUMBER(b)         (b & 0x01)

/* b4 is block type independent and indicates CID following */
#define ISO14443_4_CID_follows(b)          (b & 0x08)

/* R-Block specific bits */
#define ISO14443_4_R_BLOCK_is_NAK(b)       (b & 0x10)
#define ISO14443_v4_NACK                   0xB2
#define ISO14443_v4_MASK_RBLOCK            0xF6


/* S-Block specific bits */
#define ISO14443_4_S_BLOCK_is_WTX(b)      ((b & 0x30) == 0x30)
#define ISO14443_4_S_BLOCK_is_DSLCT(b)    ((b & 0x30) == 0x00)

#define ISO14443_4_has_CID_bit(byte)   (byte & ISO14443_4_MASK_CID)

/* The for MSB of an PCB */
enum Iso14443_Block {
	I_BLK      = 0x0,
	I_BLK_C    = 0x1,
	R_NAK      = 0xb,
	R_ACK      = 0xa,
	S_WTX      = 0xf,
	S_DSESELCT = 0xc,
};

enum RequestType {
	INVALID        = 0x00,
	CAPDU_COMPLETE = 0x01,
	CAPDU_TRANSFER,
	I_BLK_PRESENCE,
	R_NAK_PRESENCE,
	RAPDU_CONTINUE,
	R_BLK_RESEND,
	S_WTX_CONFIRM,
};

typedef struct iso14443_block Block;
typedef struct iso14443_pcb   Pcb;
typedef enum   Iso14443_Block BlockType;

struct __packed iso14443_pcb {
	BlockType type  : 4;
	u8 with_cid     : 1;
	u8 with_nad     : 1;
	u8 __rfu        : 1;
	u8 block_number : 1;
};

struct __packed iso14443_block {
	union {
		u8  pcb_plain;
		Pcb pcb;
	};
	u8  cid;
	u8  nad;
	/* context sensitive request type */
	enum RequestType type : 4;
	const u8  *INF;
	u16 INF_size;
};

PUBLIC u8 iso14443_parse_request(Block *, const Array *, u8 *);

PUBLIC u32 iso14443_fwt_in_us(u8);

static inline void
iso14443_block_reset(Block *blk) {
	memset(blk, 0x00, sizeof(*blk));
}

static inline bool
is_iso14443_r_block(u8 pcb) {
	/* A full ISO/IEC compatible R-Block matches bit mask 0y101__01_ */
	return ((pcb & ISO14443_4_R_BLOCK_MASK) == ISO14443_4_R_BLOCK_BITS);
}

static inline bool
is_iso14443_i_block(u8 pcb) {
	/* A full ISO/IEC compatible I-Block matches bit mask 0y101__01_ */
	return ((pcb & ISO14443_4_I_BLOCK_MASK) == ISO14443_4_I_BlOCK_BITS);
}

static inline bool
is_iso14443_s_block(u8 pcb) {
	return ((pcb & ISO14443_4_S_BLOCK_MASK) == ISO14443_4_S_BLOCK_BITS);
}
