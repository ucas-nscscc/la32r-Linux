/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Author: Huacai Chen <chenhuacai@loongson.cn>
 *
 * Copyright (C) 2020-2021 Loongson Technology Corporation Limited
 */
#ifndef __ASM_VDSO_GETTIMEOFDAY_H
#define __ASM_VDSO_GETTIMEOFDAY_H

#ifndef __ASSEMBLY__

#include <asm/barrier.h>
#include <asm/unistd.h>
#include <asm/vdso/vdso.h>

#define VDSO_HAS_CLOCK_GETRES		1

static __always_inline long gettimeofday_fallback(
				struct __kernel_old_timeval *_tv,
				struct timezone *_tz)
{
	register struct __kernel_old_timeval *tv asm("a0") = _tv;
	register struct timezone *tz asm("a1") = _tz;
	register long nr asm("a7") = __NR_gettimeofday;
	register long ret asm("v0");

	asm volatile(
	"       syscall 0\n"
	: "=r" (ret)
	: "r" (nr), "r" (tv), "r" (tz)
	: "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
	  "$t8", "memory");

	return ret;
}

static __always_inline long clock_gettime_fallback(
					clockid_t _clkid,
					struct __kernel_timespec *_ts)
{
	register clockid_t clkid asm("a0") = _clkid;
	register struct __kernel_timespec *ts asm("a1") = _ts;
	register long nr asm("a7") = __NR_clock_gettime;
	register long ret asm("v0");

	asm volatile(
	"       syscall 0\n"
	: "=r" (ret)
	: "r" (nr), "r" (clkid), "r" (ts)
	: "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
	  "$t8", "memory");

	return ret;
}

static __always_inline int clock_getres_fallback(
					clockid_t _clkid,
					struct __kernel_timespec *_ts)
{
	register clockid_t clkid asm("a0") = _clkid;
	register struct __kernel_timespec *ts asm("a1") = _ts;
	register long nr asm("a7") = __NR_clock_getres;
	register long ret asm("v0");

	asm volatile(
	"       syscall 0\n"
	: "=r" (ret)
	: "r" (nr), "r" (clkid), "r" (ts)
	: "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
	  "$t8", "memory");

	return ret;
}
#ifdef CONFIG_32BIT
static __always_inline u64 __arch_get_hw_counter(s32 clock_mode,
						 const struct vdso_data *vd)
{
	unsigned int count;
	unsigned int count1;
	u64 res;

	__asm__ __volatile__(
	"	rdcntvl.w %0\n"
	"   rdcntvh.w %1\n "
	: "=r" (count), "=r"(count1));

	res = ((u64)count1 <<32)| count;
	return res;
}

#else
static __always_inline u64 __arch_get_hw_counter(s32 clock_mode,
						 const struct vdso_data *vd)
{
	unsigned int count;

	__asm__ __volatile__(
	"	rdtime.d %0, $zero\n"
	: "=r" (count));

	return count;
}
#endif

static inline bool loongarch_vdso_hres_capable(void)
{
	return true;
}
#define __arch_vdso_hres_capable loongarch_vdso_hres_capable

static __always_inline const struct vdso_data *__arch_get_vdso_data(void)
{
	return get_vdso_data();
}

#endif /* !__ASSEMBLY__ */

#endif /* __ASM_VDSO_GETTIMEOFDAY_H */
