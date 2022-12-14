/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020-2021 Loongson Technology Corporation Limited
 */
#ifndef _ASM_HARDIRQ_H
#define _ASM_HARDIRQ_H

#include <linux/cache.h>
#include <linux/threads.h>
#include <linux/irq.h>

extern void ack_bad_irq(unsigned int irq);
#define ack_bad_irq ack_bad_irq

#define NR_IPI	2

typedef struct {
	unsigned int ipi_irqs[NR_IPI];
	unsigned int __softirq_pending;
} ____cacheline_aligned irq_cpustat_t;
DECLARE_PER_CPU_ALIGNED(irq_cpustat_t, irq_stat);
#ifdef CONFIG_64BIT
#define __ARCH_IRQ_STAT
#endif
#endif /* _ASM_HARDIRQ_H */
