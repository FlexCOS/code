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
#include <io/stream.h>
#include <apdu.h>
#include <channel.h>

#define BLOCK_LEN	8

#define INIT_ADDR			0
/*
 * Length of the serial number.
 */
#define INIT_LEN			8
/*
 * EEPROM address of the state of the PRNG.
 */
#define STATE_ADDR		(INIT_ADDR+INIT_LEN)
/*
 * Length of the state of the PRNG. (Counter[8], State[8], Key[16])
 */
#define STATE_LEN		8+8+16
/*
 * Magic value. (Golden number * 2^31)
 */
#define DELTA	0x9E3779B9
/*
 * Number of rounds.
 */
#define ROUNDS	32

/*
 * This array holds the values to setup the random number generator.
 */
u8 state[40] = {
	/* INIT_ADDR */
	0x23, 0xF3, 0xA1, 0x8C, 0x49, 0xA0, 0xD5, 0x26,
	/* STATE_ADDR */
    /* Counter */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* State */
    0x2C, 0x01, 0xAA, 0x00, 0xD5, 0xC0, 0x16, 0x9D,
    /* Key */
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/*
 * Encryption with the TEA Algorithm.
 * Originally from SOSSE.
 * Modifies 8 bytes beginning at *v.
 */
void tea_enc( u32 *v, const u32 *k )
{
	u32 y, z;
	u32 sum=0;
	u8 n=ROUNDS;

	y=v[0]; z=v[1];

	while(n-->0) {
		sum += DELTA;
		y += ((z<<4)+k[0]) ^ (z+sum) ^ ((z>>5)+k[1]);
		z += ((y<<4)+k[2]) ^ (y+sum) ^ ((y>>5)+k[3]);
	}

	v[0]=y; v[1]=z;
}

/**
 * Create BLOCK_SIZE bytes of random data into dst.
 * Originally from SOSSE.
 */
bool rnd_getBlock( u8 *dst )
{
	u32 block[2], key[4];
	u8 i;

	for(i = 0; i < INIT_LEN; ++i) {
		*(((u8*)block) + i) = state[INIT_ADDR + i];
	}
	for(i = 0; i < sizeof(key); ++i) {
		*(((u8*)key) + i) = state[STATE_ADDR + 16 + i];
	}

	key[2]=key[1];
	key[3]=key[0];

	tea_enc( block, key );

	for(i = 0; i < STATE_LEN; ++i) {
		state[STATE_ADDR + i] = *(((u8*)block) + i % sizeof(block));
	}

	tea_enc( block, key );

	memcpy( dst, block, sizeof(block) );

	return TRUE;
}

PUBLIC sw_t
cmd_get_challenge(const CmdAPDU *apdu)
{
	u16 partial, blocks;
	u8  i, s, b, buf[BLOCK_LEN];

	if (apdu->Lc)
		return 0x0001;

	if (!apdu->Le)
		return 0x0001;
	
	partial = apdu->Le % sizeof(buf);
	blocks  = apdu->Le / sizeof(buf);

	/* ACK */
	stream_put(current->response, apdu->header->INS);

	/* for each block */
	for (b = 0; b <= blocks; b++) {
		rnd_getBlock(buf);

		s = b == blocks ? partial : sizeof(buf);

		for (i = 0; i < s; i++) {
			/* Data */
			stream_put(current->response, buf[i]);
		}
	}

	return SW__OK;
}
