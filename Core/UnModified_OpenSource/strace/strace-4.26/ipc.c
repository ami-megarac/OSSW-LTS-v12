/*
 * Copyright (c) 2016 Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (c) 1999-2018 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "defs.h"
#include "xlat/ipccalls.h"

SYS_FUNC(ipc)
{
	unsigned int call = tcp->u_arg[0];
	unsigned int version = call >> 16;
	call &= 0xffff;

	if (version)
		tprintf("%u<<16|", version);

	printxval_u(ipccalls, call, NULL);

	unsigned int i;
	for (i = 1; i < tcp->s_ent->nargs; ++i)
		tprintf(", %#" PRI_klx, tcp->u_arg[i]);

	return RVAL_DECODED;
}
