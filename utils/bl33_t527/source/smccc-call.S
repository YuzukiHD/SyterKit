#include <linkage.h>

ENTRY(__sunxi_smc_call)
	stmfd sp!, {r4-r12, lr}	@ save reg state
	.arch_extension sec
	smc #0
	ldmfd sp!, {r4-r12, pc}	@ restore saved regs and return
ENDPROC(__sunxi_smc_call)

