/* SPDX-License-Identifier: Apache-2.0 */

void abort(void) {
    while (1)
        ;
}

int raise(int signum) {
    return 0;
}

/* Dummy function to avoid linker complaints */
void __aeabi_unwind_cpp_pr0(void) {
}