# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux/LoongArch kernel.
#

extra-y		:= head.o vmlinux.lds

obj-y		+= cmpxchg.o elf.o entry.o genex.o idle.o irq.o \
		   process.o ptrace.o reset.o setup.o signal.o \
		   syscall.o time.o topology.o traps.o unaligned.o \
		   cmdline.o switch.o vdso.o

obj-$(CONFIG_ACPI)		+= acpi.o

obj-$(CONFIG_EFI) 		+= efi.o

obj-$(CONFIG_MODULES)		+= module.o

obj-$(CONFIG_CPU_HAS_FPU)	+= fpu.o

obj-$(CONFIG_SMP)		+= smp.o

obj-$(CONFIG_EARLY_PRINTK)	+= early_printk.o

obj-$(CONFIG_32BIT)		+= cpu-probe32.o scall32.o

obj-$(CONFIG_64BIT)		+= scall64.o

obj-$(CONFIG_PROC_FS)		+= proc.o

CPPFLAGS_vmlinux.lds		:= $(KBUILD_CFLAGS)
