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

/* ISO/IEC - 7816-4: Security Condition Byte */
#define SEC_COND__NEVER  0xFF
#define SEC_COND__NOTSET 0x00

#define SEC_COND__ALL     0x80 /* b8 denotes all conditions to be satisfied */
#define SEC_COND__SM      0x40 /* b7 require Secure Messaging */
#define SEC_COND__EXTAUTH 0x20 /* b6 require external authentication */
#define SEC_COND__USRAUTH 0x10 /* b5 require user authentication */

#define SEC_COND__SEID    0x0F /* b1 to b4 reference an SEID not equal to zero or 0xF */


/* ISO/IEC - 7816-4: Security Condition Data Object */

enum TAGS_SEC_COND_DATA_OBJECT {
	SCDO_NEVER = 0x90,
	SCDO_ALWAYS = 0x97,
	SCDO_BYTE   = 0x9E,
	SCDO_A4     = 0xA4, // External or user authentication depending on the usage qualifier
	SCDO_B4     = 0xB4, // SM in command and / or response depending on the usage qualifier
	SCDO_B6     = 0xB6, 
	SCDO_B8     = 0xB8,
	SCDO_A0     = 0xA0, // Security condition data objects At least one security condition shall be fulfilled (OR template)
	SCDO_A7     = 0xA7, // Security condition data objects Inversion of the security conditions (NOT template)
	SCDO_AF     = 0xAF, // Security condition data objects Every security condition shall be fulfilled (AND template)
}
