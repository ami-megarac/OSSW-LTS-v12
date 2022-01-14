/*
 * Copyright (c) 2015-2018 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* Return -1 on error or 1 on success (never 0!). */
static int
arch_get_syscall_args(struct tcb *tcp)
{
	unsigned int i;

	for (i = 0; i < tcp->s_ent->nargs; ++i)
		if (upeek(tcp, (5 + i) * 4, &tcp->u_arg[i]) < 0)
			return -1;
	return 1;
}
