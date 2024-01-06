.align 0
.globl __sunxi_smc_call ;

__sunxi_smc_call:
	stmfd	sp!, {r4-r12, lr}	@ save reg state
	.arch_extension sec
	smc #0
	ldmfd	sp!, {r4-r12, pc}	@ restore saved regs and return

.type __sunxi_smc_call 2 ;
.size __sunxi_smc_call, .-__sunxi_smc_call