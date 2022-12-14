/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020-2021 Loongson Technology Corporation Limited
 */

#include <asm/asmmacro.h>
#include <asm/export.h>
#include <asm/regdef.h>
#include <asm/asm.h>

/*
 * void *rmemcpy(void *dst, const void *src, size_t n)
 *
 * a0: dst
 * a1: src
 * a2: n
 */
SYM_FUNC_START(rmemcpy)
	move	a3, a0
	beqz	a2, 2f

	INT_ADDU	a0, a0, a2
	INT_ADDU	a1, a1, a2

1:	ld.b	t0, a1, -1
	st.b	t0, a0, -1
	INT_ADDIU	a0, a0, -1
	INT_ADDIU	a1, a1, -1
	INT_ADDIU	a2, a2, -1
	bgt	a2, zero, 1b

2:	move	v0, a3
	jr	ra
SYM_FUNC_END(rmemcpy)

SYM_FUNC_START(memmove)
	blt	a0, a1, 1f	/* dst < src, memcpy */
	blt	a1, a0, 2f	/* src < dst, rmemcpy */
	jr	ra		/* dst == src, return */

1:	b	memcpy

2:	b	rmemcpy
SYM_FUNC_END(memmove)

EXPORT_SYMBOL(memmove)
