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

PUBLIC sw_t cmd__not_supported(const CmdAPDU *);
PUBLIC sw_t cmd__wrong_p1_p2(const CmdAPDU *);

/* some APDU commands returning a status word */
PUBLIC sw_t cmd_select__by_fid(const CmdAPDU *);
PUBLIC sw_t cmd_select__by_path(const CmdAPDU *);
PUBLIC sw_t cmd_select__by_dfname(const CmdAPDU *);

PUBLIC sw_t cmd_mse__set(const CmdAPDU *);
PUBLIC sw_t cmd_mse__store(const CmdAPDU *);
PUBLIC sw_t cmd_mse__restore(const CmdAPDU *);
PUBLIC sw_t cmd_mse__erase(const CmdAPDU *);

PUBLIC sw_t cmd_read_binary_b0__from_current_EF(const CmdAPDU *);
PUBLIC sw_t cmd_read_binary_b0__from_short_EF_id(const CmdAPDU *);

PUBLIC sw_t cmd_read_binary_b1__from_EF_id(const CmdAPDU *);
PUBLIC sw_t cmd_read_binary_b1__from_short_EF_id(const CmdAPDU *);

PUBLIC sw_t cmd_read_record__current_ef(const CmdAPDU *);
PUBLIC sw_t cmd_read_record__with_sfi(const CmdAPDU *);

PUBLIC sw_t cmd_write_record__current_ef(const CmdAPDU *);
PUBLIC sw_t cmd_write_record__with_sfi(const CmdAPDU *);

PUBLIC sw_t cmd_get_challenge(const CmdAPDU *);

PUBLIC sw_t cmd_file_create__with_sfi(const CmdAPDU *);
PUBLIC sw_t cmd_file_create__from_fcp(const CmdAPDU *);

PUBLIC sw_t cmd_ec2ps_start(const CmdAPDU *);
PUBLIC sw_t cmd_ec2ps_finish(const CmdAPDU *);
