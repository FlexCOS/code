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

struct p1_filter;
struct p2_filter;
struct ins_filter;

typedef struct p1_filter  FilterP1;
typedef struct p2_filter  FilterP2;
typedef struct ins_filter FilterIns;

/**
 * Functions of type 'fp_matcher' may be used for pattern matching through
 * APDU-Registry lookup.
 */
typedef bool (*fp_matcher)(u8 _val, u8 _pat);

typedef struct __packed {
	fp_matcher   match;
	u8           pattern;
} Pattern;

/**
 * @brief The structures p1_filter and p2_filter have the same memory layout as 
 * this structure.
 *
 * So we can use a common filter matching facility and casting to this type.
 *
 * @see find_match 
 */
typedef struct __packed {
	Pattern p;
	u8      _unused;
	void    *_opaque;
} Filter;

struct __packed p1_filter {
	Pattern p;
	u8 n;
	struct p2_filter *p2f;
};

struct __packed p2_filter {
	Pattern p;
	// TODO introduce flags: 
	// .require = Lc | Le
	// .has_not = Lc | Le
	u8 require : 4;
	u8 has_not : 4;
	fp_handle_cmd_apdu cmd;
};

struct __packed ins_filter {
	const u8               ins;
	const u8               nfilter;
	const FilterP1 *const  filter;
};

/** Constructor macro for 'struct p1_filter' */
#define PATTERN_P1(m, pat, arr) { \
	.p.match = m,                   \
	.p.pattern = pat,               \
	.n = sizeof(arr)/sizeof(*arr),  \
	.p2f = arr                      \
}

/** Constructor macro for 'struct p2_filter' */
#define PATTERN_P2(m, pat, c, arr...) { \
	.p.match = m,           \
	.p.pattern = pat,       \
	.cmd = c, ##arr         \
}

/** Constructor macro for 'struct ins_filter' */
#define INSTRUCTION( i, arr ) {       \
	.ins = i,                           \
	.nfilter = sizeof(arr)/sizeof(*arr),\
	.filter = arr                       \
}

/* ========================================================================== */
/** TODO explain apdu_registry
 *//* ======================================================================= */
extern const FilterIns apdu_registry[];
extern const u8        apdu_registry_size;
