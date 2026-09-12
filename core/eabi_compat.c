/* SPDX-License-Identifier: GPL-2.0+ */

void abort(void)
{
	while (1)
		;
}

int raise(int signum)
{
	(void)signum;
	return 0;
}

void __aeabi_unwind_cpp_pr0(void)
{
}
