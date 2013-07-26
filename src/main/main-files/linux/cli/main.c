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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <const.h>
#include <types.h>
#include <modules.h>
#include <channel.h>

#include <fs/smartfs.h>
#include <fs/some/somefs.h>
#include <io/dev.h>
#include <ram_dev.h>
#include <flexcos.h>

#include <pipe_io.h>

PRIVATE const char *HLINE = "--------------------------------------------------------------------------------";

#define SIZE   65535
PRIVATE char   buff[SIZE];

err_t
hal_mdev__use_ram(void)
{
	err_t err;
	struct mem_dev dev;

	err = ram_dev_init(&dev);
	if (err) return err;

	return module_hal_mdev_set(&dev);
}

err_t
mount_somefs(void)
{
	err_t err;

	/* since we use ram_dev above, formating fs is needed anyway */
	if ((err = somefs_mkfs(hal_mdev))
	||  (err = smartfs_mount_root(hal_mdev, somefs_mount)))
	{
		return err;
	}

	return E_GOOD;
}


MODULES(pipe_io,
        channel_setup,
        hal_mdev__use_ram,
	mount_somefs);

int
main(void)
{
	u16 i;
	u16 l_input;
	size_t recv;
	u16 l_cmd;
	u32 v;
	extern char buff[];

	u8   *apdu = (u8 *) buff;

	/* setup basic hardware */
	if (modules_init() != E_GOOD) halt();

	loop {
		puts(HLINE);
		puts("Command APDU: ");
		fgets(buff, SIZE, stdin);

		l_input = strlen(buff) - 1;
		if (l_input % 2) {
			printf("Invalid input length\n");
			continue;
		}

		for (i = 0, l_cmd = 0;
		     i < l_input;
		     i += 2, l_cmd++)
		{
			if (!sscanf(&buff[i], "%2x", &v)) {
				printf( "Invalid input at pos %d: %2s\n", i, &buff[i]);
				continue;
			}

			apdu[l_cmd] = (u8) v;
		}

		pipe_term_write(apdu, l_cmd);

		puts("\n");
		flexcos_one_shot();

		recv = pipe_term_read(apdu, SIZE);

		printf("Response APDU (%d bytes):\n", (u16) recv);

		for (i = 0; i < recv; i++) {
			if (i && i % 8 == 0)
				printf("\n");
			if (i % 8 == 0)
				printf("%04d: ", i+1);

			printf(" %02x", apdu[i]);
		}
		printf("\n");

	}
}
