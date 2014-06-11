#include <asm/assembler.h>

/*
 * Interrupt handling.  Preserves r7, r8, r9
 */
	.macro	arch_irq_handler_default
	get_irqnr_preamble r6, lr
1:	get_irqnr_and_base r0, r2, r6, lr
	movne	r1, sp
	@
	@ routine called with r0 = irq number, r1 = struct pt_regs *
	@
	adrne	lr, BSYM(1b)
	bne	asm_do_IRQ
#ifndef CONFIG_ARCH_ARMADA370
#if (defined(CONFIG_ARCH_ARMADA_XP) || defined(CONFIG_ARCH_ARMADA370)) && defined(CONFIG_PERF_EVENTS) && defined(CONFIG_HW_PERF_EVENTS)
	test_for_pmuirq r0, r2, r6, lr
	movne	r0, sp
	adrne	lr, BSYM(1b)
	bne	do_mrvl_pj4b_pmu_event
#endif
#endif
#ifdef CONFIG_SMP
	/*
	 * XXX
	 *
	 * this macro assumes that irqstat (r2) and base (r6) are
	 * preserved from get_irqnr_and_base above
	 */
#ifdef CONFIG_ARCH_ARMADA_XP
#ifdef CONFIG_MV_IPC_DRIVER
	test_for_ipc r0, r2, r6, lr
	movne	r1, sp
	adrne	lr, BSYM(1b)
	bne	do_ipc_rx_irq
#endif

#endif
	ALT_SMP(test_for_ipi r0, r2, r6, lr)
	ALT_UP_B(9997f)
	movne	r1, sp
	adrne	lr, BSYM(1b)
	bne	do_IPI
#endif
9997:
	.endm

	.macro	arch_irq_handler, symbol_name
	.align	5
	.global \symbol_name
\symbol_name:
	mov	r8, lr
	arch_irq_handler_default
	mov     pc, r8
	.endm
