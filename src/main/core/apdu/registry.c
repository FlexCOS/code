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

#include <apdu.h>

#include "commands.h"
#include "registry.h"


static bool _match_all(u8 value, u8 pattern)
{
	return true;
}
static bool _match_and(u8 value, u8 pattern)
{
	return pattern & value;
}
static bool _match_nand(u8 value, u8 pattern)
{
	return ~(pattern & value);
}
static bool _match_equal(u8 value, u8 pattern)
{
	return value == pattern;
}

static bool _match_zero_on_rshift(u8 value, u8 n)
{
	return 0 == (value >> n);
}
static bool _match_zero_on_lshift(u8 value, u8 n)
{
	return 0 == (value << n);
}
/** The pattern 'mask' marks all bits allowed to be set. Bits set to zero must
 *  not be set in value to match true.
 */
static bool _match_mask(u8 value, u8 mask)
{
	return !(value & ~mask) && value & mask;
}
/* matches only if at least one of selected bits in mask is set.
 * but returns
 * false if value equals mask */
static bool _match_mask_neq(u8 value, u8 mask)
{
	return !(value & ~mask) && ((value & mask) != mask);
}
static bool _match_any_bit(u8 value, u8 mask)
{
	return (value | ~mask) ^ ~mask;
}
static bool _match_all_bits(u8 value, u8 mask)
{
	return mask == ((value | ~mask) ^ ~mask);
}
static bool _match_not_set(u8 value, u8 mask)
{
	return !(value & mask);
}

/* ========================================================================== *
 *  APDU - Command Registry
 * ========================================================================== */

/* ----- Instruction: SELECT -------------------------------------------------*/
/** */
static FilterP2 __chosen_select_by_fid[] = {
	PATTERN_P2(_match_all, 0x00, cmd_select__by_fid)
};
static FilterP2 __chosen_select_by_dfname[] = {
	PATTERN_P2(_match_all, 0x00, cmd_select__by_dfname)
};
static FilterP2 __chosen_select_by_path[] = {
	PATTERN_P2(_match_all, 0x00, cmd_select__by_path),
};


static FilterP1 _chosen_select[] = {
	PATTERN_P1(_match_zero_on_rshift,    2, __chosen_select_by_fid),
	PATTERN_P1(_match_equal,          0x04, __chosen_select_by_dfname),
	PATTERN_P1(_match_mask,           0x09, __chosen_select_by_path)
};
/* -------------------------------------------------------------------------- */
/* ----- Get Challange -------------------------------------------------------*/
/* -------------------------------------------------------------------------- */
static FilterP2 __chosen_get_challenge__common[] = {
	PATTERN_P2(_match_equal, 0x00, cmd_get_challenge),
};
static FilterP2 __chosen_get_challenge__unsupported[] = {
	PATTERN_P2(_match_equal, 0x00, cmd__not_supported),
};

static FilterP1  _chosen_get_challenge[] = {
	PATTERN_P1(_match_all,     0x00, __chosen_get_challenge__common),
	PATTERN_P1(_match_not_set, 0x60, __chosen_get_challenge__unsupported),
};
/* -------------------------------------------------------------------------- */
/* ----- Read Binary - with P1/P2 encoded offset ---------------------------- */
/* -------------------------------------------------------------------------- */
static FilterP2 __chosen_read_binary__short_EF[] = {
	PATTERN_P2(_match_all, 0x00, cmd_read_binary_b0__from_short_EF_id)
};
static FilterP2 __chosen_read_binary__current_EF[] = {
	PATTERN_P2(_match_all, 0x00, cmd_read_binary_b0__from_current_EF)
};

static FilterP1  _chosen_read_binary_b0[] = {
	PATTERN_P1(_match_not_set, 0x80, __chosen_read_binary__current_EF),
	PATTERN_P1(_match_not_set, 0x60, __chosen_read_binary__short_EF),
};
/* -------------------------------------------------------------------------- */
/* ----- Read Binary - with data encoded offset ----------------------------- */
/* -------------------------------------------------------------------------- */
static FilterP2 __chosen_read_binary__select_identifier[] = {
	PATTERN_P2(_match_mask_neq, 0x1F, cmd_read_binary_b1__from_short_EF_id),
	PATTERN_P2(_match_all,      0x00, cmd_read_binary_b1__from_EF_id),
};
static FilterP2 __chosen_read_binary__by_EF_identifier[] = {
	PATTERN_P2(_match_all, 0x00, cmd_read_binary_b1__from_EF_id),
};

static FilterP1  _chosen_read_binary_b1[] = {
	PATTERN_P1(_match_equal, 0x00, __chosen_read_binary__select_identifier),
	PATTERN_P1(_match_all,   0x00, __chosen_read_binary__by_EF_identifier),
};
/* -------------------------------------------------------------------------- */
/* ----- Manage Security Environment ---------------------------------------- */
/* -------------------------------------------------------------------------- */
static FilterP2 __chosen_mse_set[] = {
	PATTERN_P2(_match_all, 0x00, cmd_mse__set)
};
static FilterP2 __chosen_mse_store[] = {
	PATTERN_P2(_match_all, 0x00, cmd_mse__store)
};
static FilterP2 __chosen_mse_restore[] = {
	PATTERN_P2(_match_all, 0x00, cmd_mse__restore)
};
static FilterP2 __chosen_mse_erase[] = {
	PATTERN_P2(_match_all, 0x00, cmd_mse__erase)
};

static FilterP1 _chosen_mse[] = {
	PATTERN_P1(_match_equal, 0xF2, __chosen_mse_store),
	PATTERN_P1(_match_equal, 0xF3, __chosen_mse_restore),
	PATTERN_P1(_match_equal, 0xF4, __chosen_mse_erase),
	PATTERN_P1(_match_and,   0x01, __chosen_mse_set),
};
/* -------------------------------------------------------------------------- */
/* ----- Record - Write ----------------------------------------------------- */
/* -------------------------------------------------------------------------- */
static FilterP2 __chosen_write_record[] = {
	PATTERN_P2(_match_and, 0xF9, cmd_write_record__with_sfi),
	PATTERN_P2(_match_all, 0x00, cmd_write_record__current_ef)
};

static FilterP1 _chosen_write_record[] = {
	PATTERN_P1(_match_all, 0x00, __chosen_write_record)
};
/* -------------------------------------------------------------------------- */
/* ----- Record - Read ------------------------------------------------------ */
/* -------------------------------------------------------------------------- */
static FilterP2 __chosen_read_record[] = {
	/* Record selection by Record Identifier is not supported */
	PATTERN_P2(_match_not_set, 0x04, cmd__not_supported),
	PATTERN_P2(_match_and, 0xF9, cmd_read_record__with_sfi),
	PATTERN_P2(_match_all, 0x00, cmd_read_record__current_ef)
};

static FilterP1 _chosen_read_record[] = {
	PATTERN_P1(_match_all, 0x00, __chosen_read_record)
};

/* -------------------------------------------------------------------------- */
/* ----- File - Create ------------------------------------------------------ */
/* -------------------------------------------------------------------------- */

/* P2 stores a short file identifier from bit 8 to 4. They must not have been
 * set or unset at all to be valid. Bits 1 to 3 are proprietary and must not be
 * set. */
static FilterP2 __chosen_file_create_no_fdb[] = {
	PATTERN_P2(_match_equal,    0x00, cmd_file_create__from_fcp),
	PATTERN_P2(_match_mask_neq, 0xF8, cmd_file_create__with_sfi),
};

/* P1 is a file descriptor byte. A value of 0x00 denotes tlv encoded FD in the 
 * command field. */
static FilterP1 _chosen_file_create[] = {
	PATTERN_P1(_match_equal, 0x00, __chosen_file_create_no_fdb)
};

/* ========================================================================== */
/*        Proprietary FlexCOS Commands                                        */
/* ========================================================================== */
static FilterP2 __chosen_ec2ps_start[] = {
	PATTERN_P2(_match_equal, 0x00, cmd_ec2ps_start)
};

static FilterP1 _chosen_ec2ps_start[] = {
	PATTERN_P1(_match_equal, 0x00, __chosen_ec2ps_start)
};

/* Finally: Set up an Instruction (handler) Lookup Table */
const FilterIns i7816_instructions[] = {
	INSTRUCTION( 0x22, _chosen_mse ),
	INSTRUCTION( 0x84, _chosen_get_challenge ),
	INSTRUCTION( 0xA4, _chosen_select ),
	INSTRUCTION( 0xB0, _chosen_read_binary_b0 ),
	INSTRUCTION( 0xB1, _chosen_read_binary_b1 ),
	INSTRUCTION( 0xB2, _chosen_read_record ),
	INSTRUCTION( 0xD2, _chosen_write_record ),
	INSTRUCTION( 0xE0, _chosen_file_create ),
};
const FilterIns flxcos_instructions[] = {
	INSTRUCTION( 0xC2, _chosen_ec2ps_start )
};

PRIVATE u8
find_match(const Filter *f, u8 n, u8 value)
{
	u8 i;
	const Pattern *p;
	for (i = 0; i < n; i++) {
		p = &(f[i].p);
		if (p->match(value, p->pattern))
			break;
	}

	return i;
}

PUBLIC fp_handle_cmd_apdu
apdu_get_cmd_handler(const CmdAPDU *capdu)
{
	const FilterP1  *filter_p1;
	const FilterP2  *filter_p2;
	const FilterIns *ins_set;
	u8 i, n;
	/* variables for binary search */
	u8 bs_first, bs_middle, bs_last;

	/* return default function, that returns a
	 * 'Not Supported' status word */
	fp_handle_cmd_apdu handler = cmd__not_supported;

	bs_first = 0;
	if (capdu->header->CLA & 0x80) {
		ins_set = flxcos_instructions;
		bs_last = LENGTH(flxcos_instructions) - 1;

	} else {
		ins_set = i7816_instructions;
		bs_last = LENGTH(i7816_instructions) - 1;
	}
	bs_middle = (bs_last - bs_first) / 2;

	while (bs_first <= bs_last) {
		/* we have found the instruction in our registry.
		 * Now it is matching time for P1 and P2... */
		if (ins_set[bs_middle].ins == capdu->header->INS) {
			filter_p1 = ins_set[bs_middle].filter;
			n         = ins_set[bs_middle].nfilter;
			i         = find_match((Filter*) filter_p1, n,
					capdu->header->P1);
			if (i == n) {
				handler = cmd__wrong_p1_p2;
				break;
			}

			filter_p2 = filter_p1[i].p2f;
			n         = filter_p1[i].n;
			i         = find_match((Filter*) filter_p2, n,
					capdu->header->P2);
			if (i < n)
				handler = filter_p2[i].cmd;
			else
				handler = cmd__wrong_p1_p2;

			break;
		}
		/* expect a sorted registry: we can do some binary search */
		else if (capdu->header->INS < ins_set[bs_middle].ins) {
			bs_last = bs_middle - 1;
		}
		else {
			bs_first = bs_middle + 1;
		}

		bs_middle = (bs_first + bs_last) / 2;
	}

	return handler;
}
