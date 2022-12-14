/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020-2021 Loongson Technology Corporation Limited
 */
#include <linux/init.h>
#include <linux/threads.h>

#include <asm/addrspace.h>
#include <asm/asm.h>
#include <asm/asmmacro.h>
#include <asm/irqflags.h>
#include <asm/regdef.h>
#include <asm/loongarchregs.h>
#include <asm/stackframe.h>

SYM_ENTRY(_stext, SYM_L_GLOBAL, SYM_A_NONE)

	__REF

SYM_CODE_START(kernel_entry)                    # kernel entry point

        /* We might not get launched at the address the kernel is linked to,
           so we jump there.  */
        la.abs          t0, 0f
        jirl            zero, t0, 0
0:
#ifdef CONFIG_USE_OF

        li.w            t1, -2
        or              t2, a1, zero
        beq             a0, t1, dtb_found

        li.w            t2, 0
dtb_found:
#endif

#if 0
	la              t0, __bss_start         # clear .bss
        PTR_S           zero, t0, 0
        la              t1, __bss_stop - LONGSIZE
1:
        PTR_ADDIU	t0, t0, LONGSIZE
        PTR_S		zero, t0, 0
        bne		t0, t1, 1b
#endif

        la              t0, fw_arg0
        PTR_S           a0, t0, 0               # firmware arguments
        la              t0, fw_arg1
        PTR_S           a1, t0, 0
        la              t0, fw_arg2
        PTR_S           a2, t0, 0
        la              t0, fw_arg3
        PTR_S           a3, t0, 0

#ifdef CONFIG_USE_OF
        la      t0, fw_passed_dtb
        PTR_S            t2, t0, 0
#endif

        /* Config direct window and set PG */
        PTR_LI          t0, 0xa0000011
        csrwr           t0, LOONGARCH_CSR_DMWIN0
        PTR_LI          t0, 0x80000001
        csrwr           t0, LOONGARCH_CSR_DMWIN1
        /* Enable PG */
        li.w            t0, 0xb0                # PLV=0, IE=0, PG=1
        csrwr           t0, LOONGARCH_CSR_CRMD

        /* KScratch3 used for percpu base, initialized as 0 */
        csrwr           zero, PERCPU_BASE_KS
        /* GPR21 used for percpu base (runtime), initialized as 0 */
        or              x0, zero, zero

        la              tp, init_thread_union
        /* Set the SP after an empty pt_regs.  */
        PTR_LI          sp, (_THREAD_SIZE - 32 - PT_SIZE)
        PTR_ADDU        sp, sp, tp
        set_saved_sp    sp, t0, t1
        PTR_ADDIU       sp, sp, -4 * SZREG      # init stack pointer

        b               start_kernel

SYM_CODE_END(kernel_entry)

#ifdef CONFIG_SMP

/*
 * SMP slave cpus entry point.	Board specific code for bootstrap calls this
 * function after setting up the stack and tp registers.
 */
SYM_CODE_START(smpboot_entry)
	PTR_LI		t0, CSR_DMW0_INIT	# UC, PLV0
	csrwr		t0, LOONGARCH_CSR_DMWIN0
	PTR_LI		t0, CSR_DMW1_INIT	# CA, PLV0
	csrwr		t0, LOONGARCH_CSR_DMWIN1
	li.w		t0, 0xb0		# PLV=0, IE=0, PG=1
	csrwr		t0, LOONGARCH_CSR_CRMD

	b		start_secondary
SYM_CODE_END(smpboot_entry)

#endif /* CONFIG_SMP */
