/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Atomic operations.
 *
 * Copyright (C) 2020-2021 Loongson Technology Corporation Limited
 */
#ifndef _ASM_ATOMIC_H
#define _ASM_ATOMIC_H

#include <linux/irqflags.h>
#include <linux/types.h>
#include <asm/barrier.h>
#include <asm/compiler.h>
#include <asm/cpu-features.h>
#include <asm/cmpxchg.h>

#define ATOMIC_INIT(i)	  { (i) }

/*
 * arch_atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define arch_atomic_read(v)	READ_ONCE((v)->counter)

/*
 * arch_atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
#define arch_atomic_set(v, i)	WRITE_ONCE((v)->counter, (i))
#ifdef CONFIG_64BIT
#define ATOMIC_OP(op, I, asm_op)					\
static inline void arch_atomic_##op(int i, atomic_t *v)			\
{									\
	__asm__ __volatile__(						\
	"am"#asm_op"_db.w" " $zero, %1, %0	\n"			\
	: "+ZB" (v->counter)						\
	: "r" (I)							\
	: "memory");							\
}

#define ATOMIC_OP_RETURN(op, I, asm_op, c_op)				\
static inline int arch_atomic_##op##_return_relaxed(int i, atomic_t *v)	\
{									\
	int result;							\
									\
	__asm__ __volatile__(						\
	"am"#asm_op"_db.w" " %1, %2, %0		\n"			\
	: "+ZB" (v->counter), "=&r" (result)				\
	: "r" (I)							\
	: "memory");							\
									\
	return result c_op I;						\
}

#define ATOMIC_FETCH_OP(op, I, asm_op)					\
static inline int arch_atomic_fetch_##op##_relaxed(int i, atomic_t *v)	\
{									\
	int result;							\
									\
	__asm__ __volatile__(						\
	"am"#asm_op"_db.w" " %1, %2, %0		\n"			\
	: "+ZB" (v->counter), "=&r" (result)				\
	: "r" (I)							\
	: "memory");							\
									\
	return result;							\
}
#endif

#ifdef CONFIG_32BIT   /* CONFIG_32BIT */
#define ATOMIC_OP(op, I, asm_op)                                        \
static __inline__ void arch_atomic_##op(int i, atomic_t * v)                 \
{                                                                       \
        int temp ;                                              \
        __asm__ __volatile__(                                   \
        "1:     ll.w        %0, %1      #atomic_" #op "  \n"    \
        "       " #asm_op " %0, %0, %2                   \n"    \
        "       sc.w        %0, %1                       \n"    \
        "       beq         %0, $r0, 1b                  \n"    \
        :"=&r" (temp) , "+ZB"(v->counter)     \
        :"r" (I)                                               \
        );                                                      \
}

#define ATOMIC_OP_RETURN(op, I, asm_op)                                    \
static __inline__ int arch_atomic_##op##_return_relaxed(int i, atomic_t * v)       \
{                                                                             \
        int result;                                                           \
        int temp;                                                             \
                                                                              \
        __asm__ __volatile__(                                                 \
                "1:     ll.w    %1, %2          # atomic_" #op "_return \n"   \
                "       " #asm_op " %0, %1, %3                          \n"   \
                "       sc.w    %0, %2                                  \n"   \
                "       beq    %0, $r0 ,1b                             \n"   \
                "       " #asm_op " %0, %1, %3                          \n"   \
                : "=&r" (result), "=&r" (temp),                               \
                  "+ZB"(v->counter)                        \
                : "r" (I));                                                  \
        return result;                                                        \
}

#define ATOMIC_FETCH_OP(op,I, asm_op)                                     \
static __inline__ int arch_atomic_fetch_##op##_relaxed(int i, atomic_t * v)        \
{                                                                             \
        int result;                                                           \
        int temp;                                                     \
                                                                      \
        __asm__ __volatile__(                                         \
        "1:     ll.w    %1, %2          # atomic_fetch_" #op "  \n"   \
        "       " #asm_op " %0, %1, %3                          \n"   \
        "       sc.w    %0, %2                                  \n"   \
        "       beq     %0, $r0 ,1b                             \n"   \
        "       add.w     %0, %1  ,$r0                            \n"   \
        : "=&r" (result), "=&r" (temp),                               \
        "+ZB" (v->counter)                          \
        : "r" (I));                                                  \
                                                                      \
        return result;                                                \
}
#endif

#ifdef CONFIG_64BIT
#define ATOMIC_OPS(op, I, asm_op, c_op)					\
	ATOMIC_OP(op, I, asm_op)					\
	ATOMIC_OP_RETURN(op, I, asm_op, c_op)				\
	ATOMIC_FETCH_OP(op, I, asm_op)
ATOMIC_OPS(add, i, add, +)
ATOMIC_OPS(sub, -i, add, +)
#endif

#ifdef CONFIG_32BIT
#define ATOMIC_OPS(op,I ,asm_op, c_op)                                          \
        ATOMIC_OP(op, I, asm_op)                                           \
        ATOMIC_OP_RETURN(op, I , asm_op)                                    \
        ATOMIC_FETCH_OP(op, I, asm_op)
ATOMIC_OPS(add, i , add.w ,+=)
ATOMIC_OPS(sub, -i , add.w ,+=)
#endif

#define arch_atomic_add_return_relaxed	arch_atomic_add_return_relaxed
#define arch_atomic_sub_return_relaxed	arch_atomic_sub_return_relaxed
#define arch_atomic_fetch_add_relaxed	arch_atomic_fetch_add_relaxed
#define arch_atomic_fetch_sub_relaxed	arch_atomic_fetch_sub_relaxed

#undef ATOMIC_OPS

#define ATOMIC_OPS(op, I, asm_op)					\
	ATOMIC_OP(op, I, asm_op)					\
	ATOMIC_FETCH_OP(op, I, asm_op)

ATOMIC_OPS(and, i, and)
ATOMIC_OPS(or, i, or)
ATOMIC_OPS(xor, i, xor)

#define arch_atomic_fetch_and_relaxed	arch_atomic_fetch_and_relaxed
#define arch_atomic_fetch_or_relaxed	arch_atomic_fetch_or_relaxed
#define arch_atomic_fetch_xor_relaxed	arch_atomic_fetch_xor_relaxed

#undef ATOMIC_OPS
#undef ATOMIC_FETCH_OP
#undef ATOMIC_OP_RETURN
#undef ATOMIC_OP

/*
 * arch_atomic_sub_if_positive - conditionally subtract integer from atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically test @v and subtract @i if @v is greater or equal than @i.
 * The function returns the old value of @v minus @i.
 */
static inline int arch_atomic_sub_if_positive(int i, atomic_t *v)
{
	int result;
	int temp;

	if (__builtin_constant_p(i)) {
		__asm__ __volatile__(
		"1:	ll.w	%1, %2		# atomic_sub_if_positive\n"
		"	addi.w	%0, %1, %3				\n"
		"	or	%1, %0, $zero				\n"
		"	blt	%0, $zero, 2f				\n"
		"	sc.w	%1, %2					\n"
		"	beq	$zero, %1, 1b				\n"
		"2:							\n"
		__WEAK_LLSC_MB
		: "=&r" (result), "=&r" (temp),
		  "+" GCC_OFF_SMALL_ASM() (v->counter)
		: "I" (-i));
	} else {
		__asm__ __volatile__(
		"1:	ll.w	%1, %2		# atomic_sub_if_positive\n"
		"	sub.w	%0, %1, %3				\n"
		"	or	%1, %0, $zero				\n"
		"	blt	%0, $zero, 2f				\n"
		"	sc.w	%1, %2					\n"
		"	beq	$zero, %1, 1b				\n"
		"2:							\n"
		__WEAK_LLSC_MB
		: "=&r" (result), "=&r" (temp),
		  "+" GCC_OFF_SMALL_ASM() (v->counter)
		: "r" (i));
	}

	return result;
}

#define arch_atomic_cmpxchg(v, o, n) (arch_cmpxchg(&((v)->counter), (o), (n)))
#define arch_atomic_xchg(v, new) (arch_xchg(&((v)->counter), (new)))

/*
 * arch_atomic_dec_if_positive - decrement by 1 if old value positive
 * @v: pointer of type atomic_t
 */
#define arch_atomic_dec_if_positive(v)	arch_atomic_sub_if_positive(1, v)
#ifdef CONFIG_64BIT
#define ATOMIC64_INIT(i)    { (i) }
/*
 * arch_atomic64_read - read atomic variable
 * @v: pointer of type atomic64_t
 *
 */
#define arch_atomic64_read(v)	READ_ONCE((v)->counter)

/*
 * arch_atomic64_set - set atomic variable
 * @v: pointer of type atomic64_t
 * @i: required value
 */
#define arch_atomic64_set(v, i)	WRITE_ONCE((v)->counter, (i))

#define ATOMIC64_OP(op, I, asm_op)					\
static inline void arch_atomic64_##op(long i, atomic64_t *v)		\
{									\
	__asm__ __volatile__(						\
	"am"#asm_op"_db.d " " $zero, %1, %0	\n"			\
	: "+ZB" (v->counter)						\
	: "r" (I)							\
	: "memory");							\
}

#define ATOMIC64_OP_RETURN(op, I, asm_op, c_op)					\
static inline long arch_atomic64_##op##_return_relaxed(long i, atomic64_t *v)	\
{										\
	long result;								\
	__asm__ __volatile__(							\
	"am"#asm_op"_db.d " " %1, %2, %0		\n"			\
	: "+ZB" (v->counter), "=&r" (result)					\
	: "r" (I)								\
	: "memory");								\
										\
	return result c_op I;							\
}

#define ATOMIC64_FETCH_OP(op, I, asm_op)					\
static inline long arch_atomic64_fetch_##op##_relaxed(long i, atomic64_t *v)	\
{										\
	long result;								\
										\
	__asm__ __volatile__(							\
	"am"#asm_op"_db.d " " %1, %2, %0		\n"			\
	: "+ZB" (v->counter), "=&r" (result)					\
	: "r" (I)								\
	: "memory");								\
										\
	return result;								\
}

#define ATOMIC64_OPS(op, I, asm_op, c_op)				      \
	ATOMIC64_OP(op, I, asm_op)					      \
	ATOMIC64_OP_RETURN(op, I, asm_op, c_op)				      \
	ATOMIC64_FETCH_OP(op, I, asm_op)

ATOMIC64_OPS(add, i, add, +)
ATOMIC64_OPS(sub, -i, add, +)

#define arch_atomic64_add_return_relaxed	arch_atomic64_add_return_relaxed
#define arch_atomic64_sub_return_relaxed	arch_atomic64_sub_return_relaxed
#define arch_atomic64_fetch_add_relaxed		arch_atomic64_fetch_add_relaxed
#define arch_atomic64_fetch_sub_relaxed		arch_atomic64_fetch_sub_relaxed

#undef ATOMIC64_OPS

#define ATOMIC64_OPS(op, I, asm_op)					      \
	ATOMIC64_OP(op, I, asm_op)					      \
	ATOMIC64_FETCH_OP(op, I, asm_op)

ATOMIC64_OPS(and, i, and)
ATOMIC64_OPS(or, i, or)
ATOMIC64_OPS(xor, i, xor)

#define arch_atomic64_fetch_and_relaxed	arch_atomic64_fetch_and_relaxed
#define arch_atomic64_fetch_or_relaxed	arch_atomic64_fetch_or_relaxed
#define arch_atomic64_fetch_xor_relaxed	arch_atomic64_fetch_xor_relaxed

#undef ATOMIC64_OPS
#undef ATOMIC64_FETCH_OP
#undef ATOMIC64_OP_RETURN
#undef ATOMIC64_OP

/*
 * arch_atomic64_sub_if_positive - conditionally subtract integer from atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic64_t
 *
 * Atomically test @v and subtract @i if @v is greater or equal than @i.
 * The function returns the old value of @v minus @i.
 */
static inline long arch_atomic64_sub_if_positive(long i, atomic64_t *v)
{
	long result;
	long temp;

	if (__builtin_constant_p(i)) {
		__asm__ __volatile__(
		"1:	ll.d	%1, %2	# atomic64_sub_if_positive	\n"
		"	addi.d	%0, %1, %3				\n"
		"	or	%1, %0, $zero				\n"
		"	blt	%0, $zero, 2f				\n"
		"	sc.d	%1, %2					\n"
		"	beq	%1, $zero, 1b				\n"
		"2:							\n"
		__WEAK_LLSC_MB
		: "=&r" (result), "=&r" (temp),
		  "+" GCC_OFF_SMALL_ASM() (v->counter)
		: "I" (-i));
	} else {
		__asm__ __volatile__(
		"1:	ll.d	%1, %2	# atomic64_sub_if_positive	\n"
		"	sub.d	%0, %1, %3				\n"
		"	or	%1, %0, $zero				\n"
		"	blt	%0, $zero, 2f				\n"
		"	sc.d	%1, %2					\n"
		"	beq	%1, $zero, 1b				\n"
		"2:							\n"
		__WEAK_LLSC_MB
		: "=&r" (result), "=&r" (temp),
		  "+" GCC_OFF_SMALL_ASM() (v->counter)
		: "r" (i));
	}

	return result;
}

#define arch_atomic64_cmpxchg(v, o, n) \
	((__typeof__((v)->counter))arch_cmpxchg(&((v)->counter), (o), (n)))
#define arch_atomic64_xchg(v, new) (arch_xchg(&((v)->counter), (new)))

/*
 * arch_atomic64_dec_if_positive - decrement by 1 if old value positive
 * @v: pointer of type atomic64_t
 */
#define arch_atomic64_dec_if_positive(v)	arch_atomic64_sub_if_positive(1, v)
#endif /* CONFIG_64BIT */
#endif /* _ASM_ATOMIC_H */
