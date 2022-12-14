/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020-2021 Loongson Technology Corporation Limited
 */
#include <asm/asm.h>
#include <asm/loongarchregs.h>
#include <asm/asm-offsets.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/thread_info.h>

#include <asm/asmmacro.h>

/*
 * task_struct *resume(task_struct *prev, task_struct *next,
 *		       struct thread_info *next_ti)
 */
	.align	5
SYM_FUNC_START(resume)
	csrrd	t1, LOONGARCH_CSR_PRMD
	LONG_SPTR	t1, a0, THREAD_CSRPRMD
	csrrd	t1, LOONGARCH_CSR_CRMD
	LONG_SPTR	t1, a0, THREAD_CSRCRMD
	csrrd	t1, LOONGARCH_CSR_ECFG
	LONG_SPTR	t1, a0, THREAD_CSRECFG
	csrrd	t1, LOONGARCH_CSR_EUEN
	LONG_SPTR	t1, a0, THREAD_CSREUEN
	cpu_save_nonscratch a0
	LONG_SPTR	ra, a0, THREAD_REG01

	/*
	 * The order of restoring the registers takes care of the race
	 * updating $28, $29 and kernelsp without disabling ints.
	 */
	move	tp, a2
	cpu_restore_nonscratch a1

	li.w	t0, _THREAD_SIZE - 32
	PTR_ADDU	t0, t0, tp
	set_saved_sp	t0, t1, t2

	LONG_LPTR	t1, a1, THREAD_CSRPRMD
	csrwr	t1, LOONGARCH_CSR_PRMD
	LONG_LPTR	t1, a1, THREAD_CSREUEN
	csrwr	t1, LOONGARCH_CSR_EUEN

	jr	ra
SYM_FUNC_END(resume)
