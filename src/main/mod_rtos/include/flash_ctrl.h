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
 *
 * @author    Alexander Münn
 */
#pragma once

#define FLASH_CTRL_CMD_RESERVED                  0x194FE018

struct flash_layout;
struct flash_ctrl_ctx {
	/* bool                       auto_write_enable; */
	struct flash_layout  *layout;
};


/* Access layout properties through Flash Controller context. */
extern const struct flash_ctrl_ctx *flash_ctrl;

PUBLIC err_t flash_ctrl_spi();

/* write buffer to page */
PUBLIC err_t flash_ctrl_write_xor(u32, const u8 *, u8);

/* read random indefinitely */
PUBLIC err_t flash_ctrl_read(u32, u8 *, u32);
PUBLIC err_t flash_ctrl_page_prog(u32, const u8 *, u16);
PUBLIC err_t flash_ctrl_write_enable(void);
PUBLIC err_t flash_ctrl_write_passthrough(u32, const u8 *, u32);
PUBLIC err_t flash_ctrl_read_passthrough(u32, u8 *, u32);
PUBLIC err_t flash_ctrl_sector_erase(u32);
PUBLIC err_t flash_ctrl_bulk_erase(void);
PUBLIC err_t flash_ctrl_read_id(u8 *, u8);

PUBLIC err_t flash_ctrl_wait(void);

