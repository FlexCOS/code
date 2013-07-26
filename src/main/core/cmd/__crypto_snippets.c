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

#include <miracl.h>

/* aes vars */
aes a;
MR_BYTE y,x,m;
char key[32];
char block[16];
char iv[16];
/* end aes vars */

void cmd_mse( void )
{
	int i;

	/* we only support encipherment and decipherment right now */
	if (header[2] != 0x81 && header[2] != 0x41) {
		sw_set( SW_WRONG_P1P2 );
		return;
	}

	/* encipher */
	if (header[2] == 0x81) {
		for (i=0;i<32;i++) key[i]=0;
		key[0]=0x01;
		key[1]=0xf3;
		key[2]=0xbc;
		key[3]=0x34;
		key[4]=0x99;
		key[5]=0x21;
		key[6]=0xf2;
		key[7]=0x00;
		key[8]=0x3f;
		key[9]=0x07;
		key[10]=0x8a;
		key[11]=0xc2;
		key[12]=0x9f;
		key[13]=0x7d;
		key[14]=0xdd;
		key[15]=0x61;
		for (i=0;i<16;i++) iv[i]=0;
		for (i=0;i<16;i++) block[i]=0;
		/* 16 = 128, 24 = 192, 32 = 256 bits */
		if (!aes_init(&a,MR_CBC,16,key,iv)) {
			sw_set( SW_REF_DATA_NOT_FOUND );
			return;
		}
	}

	/* decipher */
	if (header[2] == 0x41) {
		for (i=0;i<32;i++) key[i]=0;
		key[0]=0x01;
		key[1]=0xf3;
		key[2]=0xbc;
		key[3]=0x34;
		key[4]=0x99;
		key[5]=0x21;
		key[6]=0xf2;
		key[7]=0x00;
		key[8]=0x3f;
		key[9]=0x07;
		key[10]=0x8a;
		key[11]=0xc2;
		key[12]=0x9f;
		key[13]=0x7d;
		key[14]=0xdd;
		key[15]=0x61;
		for (i=0;i<16;i++) iv[i]=0;
		for (i=0;i<16;i++) block[i]=0;
		/* 16 = 128, 24 = 192, 32 = 256 bits */
		if (!aes_init(&a,MR_CBC,16,key,iv)){
			sw_set( SW_REF_DATA_NOT_FOUND );
			return;
		}
	}

	sw_set( SW_OK );
}

void cmd_pso( void )
{
	int i;
	u8 le;

	/* we only support encipherment and decipherment right now */
	if (header[2] != 0x82 && header[2] != 0x84 && header[2] != 0x86 && header[2] != 0x80) {
		sw_set( SW_WRONG_P1P2 );
		return;
	}

	if (header[4] < 1) {
		sw_set( SW_WRONG_LEN );
		return;
	}

	/* decipher */
	if (header[2] == 0x80) {
		u8 tag, len;
		tag = hal_io_recByteT0();
		len = hal_io_recByteT0();
		for (i=0; i < len; ++i) {
			block[i] = hal_io_recByteT0();
		}
		le = hal_io_recByteT0();

		if (le == 0) {
			le = (u8)256;
		}

		if (le < 18) {
			sw_set( SW_WRONG_LE );
			return;
		}
		aes_decrypt(&a,block);

		hal_io_sendByteT0( header[3] );
		hal_io_sendByteT0( 0x10 );

		/* Data */
		for( i=0; i<16; i++ ) {
			hal_io_sendByteT0( block[i] );
		}
	}

	/* encipher */
	if (header[3] == 0x80) {
		for (i=0; i < header[4]; ++i) {
			block[i] = hal_io_recByteT0();
		}
		le = hal_io_recByteT0();

		if (le == 0) {
			le = (u8)256;
		}

		if (le < 18) {
			sw_set( SW_WRONG_LE );
			return;
		}
		aes_encrypt(&a,block);
		hal_io_sendByteT0( header[2] );
		hal_io_sendByteT0( 0x10 );

		/* Data */
		for( i=0; i<16; i++ ) {
			hal_io_sendByteT0( block[i] );
		}
	}

	sw_set( SW_OK );
}
