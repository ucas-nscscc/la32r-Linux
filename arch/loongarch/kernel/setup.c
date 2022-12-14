// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020-2021 Loongson Technology Corporation Limited
 *
 * Derived from MIPS:
 * Copyright (C) 1995 Linus Torvalds
 * Copyright (C) 1995 Waldorf Electronics
 * Copyright (C) 1994, 95, 96, 97, 98, 99, 2000, 01, 02, 03  Ralf Baechle
 * Copyright (C) 1996 Stoned Elipot
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) 2000, 2001, 2002, 2007         Maciej W. Rozycki
 */
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/export.h>
#include <linux/screen_info.h>
#include <linux/memblock.h>
#include <linux/initrd.h>
#include <linux/root_dev.h>
#include <linux/highmem.h>
#include <linux/console.h>
#include <linux/pfn.h>
#include <linux/sizes.h>
#include <linux/device.h>
#include <linux/dma-map-ops.h>
#include <linux/swiotlb.h>

#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <asm/cache.h>
#include <asm/cpu.h>
#include <asm/dma.h>
#include <asm/sections.h>
#include <asm/setup.h>

#ifdef CONFIG_SMP
#include <asm/smp.h>
#endif

unsigned long kernelsp[NR_CPUS];
unsigned long fw_arg0, fw_arg1, fw_arg2, fw_arg3;
struct cpuinfo_loongarch cpu_data[NR_CPUS] __read_mostly;

EXPORT_SYMBOL(cpu_data);

#ifdef CONFIG_VT
struct screen_info screen_info;
#endif

/*
 * Setup information
 *
 * These are initialized so they are in the .data section
 */

char __initdata arcs_cmdline[COMMAND_LINE_SIZE];
static char __initdata command_line[COMMAND_LINE_SIZE];

/*
 * loongarch_io_port_base is the begin of the address space to which x86 style
 * I/O ports are mapped.
 */
unsigned long loongarch_io_port_base = -1;
EXPORT_SYMBOL(loongarch_io_port_base);

static struct resource code_resource = { .name = "Kernel code", };
static struct resource data_resource = { .name = "Kernel data", };
static struct resource bss_resource = { .name = "Kernel bss", };

unsigned long __kaslr_offset __ro_after_init;
EXPORT_SYMBOL(__kaslr_offset);

static void *detect_magic __initdata = detect_memory_region;

void __init detect_memory_region(phys_addr_t start, phys_addr_t sz_min, phys_addr_t sz_max)
{
	void *dm = &detect_magic;
	phys_addr_t size;

	for (size = sz_min; size < sz_max; size <<= 1) {
		if (!memcmp(dm, dm + size, sizeof(detect_magic)))
			break;
	}

	pr_debug("Memory: %lluMB of RAM detected at 0x%llx (min: %lluMB, max: %lluMB)\n",
		((unsigned long long) size) / SZ_1M,
		(unsigned long long) start,
		((unsigned long long) sz_min) / SZ_1M,
		((unsigned long long) sz_max) / SZ_1M);

	memblock_add(start, size);
}

/*
 * Manage initrd
 */
#ifdef CONFIG_BLK_DEV_INITRD
static int __init rd_start_early(char *p)
{
	unsigned long start = memparse(p, &p);

#ifdef CONFIG_64BIT
	/* Guess if the sign extension was forgotten by bootloader */
	if (start < CAC_BASE)
		start = (int)start;
#endif
	initrd_start = start;
	initrd_end += start;
	return 0;
}
early_param("rd_start", rd_start_early);

static int __init rd_size_early(char *p)
{
	initrd_end += memparse(p, &p);
	return 0;
}
early_param("rd_size", rd_size_early);

static unsigned long __init init_initrd(void)
{
	/*
	 * Board specific code or command line parser should have
	 * already set up initrd_start and initrd_end. In these cases
	 * perfom sanity checks and use them if all looks good.
	 */
	if (!initrd_start || initrd_end <= initrd_start)
		goto disable;

	if (initrd_start & ~PAGE_MASK) {
		pr_err("initrd start must be page aligned\n");
		goto disable;
	}
	if (initrd_start < PAGE_OFFSET) {
		pr_err("initrd start < PAGE_OFFSET\n");
		goto disable;
	}

	ROOT_DEV = Root_RAM0;

	return 0;
disable:
	initrd_start = 0;
	initrd_end = 0;
	return 0;
}

static void __init finalize_initrd(void)
{
	unsigned long size = initrd_end - initrd_start;

	if (size == 0) {
		pr_info("Initrd not found or empty");
		goto disable;
	}
	if (__pa(initrd_end) > PFN_PHYS(max_low_pfn)) {
		pr_err("Initrd extends beyond end of memory");
		goto disable;
	}


	memblock_reserve(__pa(initrd_start), size);
	initrd_below_start_ok = 1;

	pr_info("Initial ramdisk at: 0x%lx (%lu bytes)\n",
		initrd_start, size);
	return;
disable:
	pr_cont(" - disabling initrd\n");
	initrd_start = 0;
	initrd_end = 0;
}

#else  /* !CONFIG_BLK_DEV_INITRD */

static unsigned long __init init_initrd(void)
{
	return 0;
}

#define finalize_initrd()	do {} while (0)

#endif

static int usermem __initdata;

static int __init early_parse_mem(char *p)
{
	phys_addr_t start, size;

	/*
	 * If a user specifies memory size, we
	 * blow away any automatically generated
	 * size.
	 */
	if (usermem == 0) {
		usermem = 1;
		memblock_remove(memblock_start_of_DRAM(),
			memblock_end_of_DRAM() - memblock_start_of_DRAM());
	}
	start = 0;
	size = memparse(p, &p);
	if (*p == '@')
		start = memparse(p + 1, &p);

	memblock_add(start, size);

	return 0;
}
early_param("mem", early_parse_mem);

static int __init early_parse_memmap(char *p)
{
	char *oldp;
	u64 start_at, mem_size;

	if (!p)
		return -EINVAL;

	if (!strncmp(p, "exactmap", 8)) {
		pr_err("\"memmap=exactmap\" invalid on LoongArch\n");
		return 0;
	}

	oldp = p;
	mem_size = memparse(p, &p);
	if (p == oldp)
		return -EINVAL;

	if (*p == '@') {
		start_at = memparse(p+1, &p);
		memblock_add(start_at, mem_size);
	} else if (*p == '#') {
		pr_err("\"memmap=nn#ss\" (force ACPI data) invalid on LoongArch\n");
		return -EINVAL;
	} else if (*p == '$') {
		start_at = memparse(p+1, &p);
		memblock_add(start_at, mem_size);
		memblock_reserve(start_at, mem_size);
	} else {
		pr_err("\"memmap\" invalid format!\n");
		return -EINVAL;
	}

	if (*p == '\0') {
		usermem = 1;
		return 0;
	} else
		return -EINVAL;
}
early_param("memmap", early_parse_memmap);

static void __init check_kernel_sections_mem(void)
{
	phys_addr_t start = __pa_symbol(&_text);
	phys_addr_t size = __pa_symbol(&_end) - start;

	if (!memblock_is_region_memory(start, size)) {
		pr_info("Kernel sections are not in the memory maps\n");
		memblock_add(start, size);
	}
}

static void __init bootcmdline_append(const char *s, size_t max)
{
	if (!s[0] || !max)
		return;

	if (boot_command_line[0])
		strlcat(boot_command_line, " ", COMMAND_LINE_SIZE);

	strlcat(boot_command_line, s, max);
}

static void __init bootcmdline_init(char **cmdline_p)
{
	boot_command_line[0] = 0;

	/*
	 * Take arguments from the bootloader at first. Early code should have
	 * filled arcs_cmdline with arguments from the bootloader.
	 */
	bootcmdline_append(arcs_cmdline, COMMAND_LINE_SIZE);

	strlcpy(command_line, boot_command_line, COMMAND_LINE_SIZE);
	*cmdline_p = command_line;

	parse_early_param();
}

/*
 * arch_mem_init - initialize memory management subsystem
 */
static void __init arch_mem_init(char **cmdline_p)
{
	if (usermem)
		pr_info("User-defined physical RAM map overwrite\n");

	check_kernel_sections_mem();

#ifndef CONFIG_NUMA
	memblock_set_node(0, PHYS_ADDR_MAX, &memblock.memory, 0);
#endif

	/*
	 * Prevent memblock from allocating high memory.
	 * This cannot be done before max_low_pfn is detected, so up
	 * to this point is possible to only reserve physical memory
	 * with memblock_reserve; memblock_alloc* can be used
	 * only after this point
	 */
	memblock_set_current_limit(PFN_PHYS(max_low_pfn));

	/*
	 * In order to reduce the possibility of kernel panic when failed to
	 * get IO TLB memory under CONFIG_SWIOTLB, it is better to allocate
	 * low memory as small as possible before plat_swiotlb_setup(), so
	 * make sparse_init() using top-down allocation.
	 */
	memblock_set_bottom_up(false);
	sparse_init();
	memblock_set_bottom_up(true);
	plat_swiotlb_setup();

	dma_contiguous_reserve(PFN_PHYS(max_low_pfn));
	memblock_dump_all();

	early_memtest(PFN_PHYS(ARCH_PFN_OFFSET), PFN_PHYS(max_low_pfn));
}

static void __init resource_init(void)
{
	u64 i;
	phys_addr_t start, end;

	code_resource.start = __pa_symbol(&_text);
	code_resource.end = __pa_symbol(&_etext) - 1;
	data_resource.start = __pa_symbol(&_etext);
	data_resource.end = __pa_symbol(&_edata) - 1;
	bss_resource.start = __pa_symbol(&__bss_start);
	bss_resource.end = __pa_symbol(&__bss_stop) - 1;

	for_each_mem_range(i, &start, &end) {
		struct resource *res;

		res = memblock_alloc(sizeof(struct resource), SMP_CACHE_BYTES);
		if (!res)
			panic("%s: Failed to allocate %zu bytes\n", __func__,
			      sizeof(struct resource));

		res->start = start;
		/*
		 * In memblock, end points to the first byte after the
		 * range while in resourses, end points to the last byte in
		 * the range.
		 */
		res->end = end - 1;
		res->flags = IORESOURCE_SYSTEM_RAM | IORESOURCE_BUSY;
		res->name = "System RAM";

		request_resource(&iomem_resource, res);

		/*
		 *  We don't know which RAM region contains kernel data,
		 *  so we try it repeatedly and let the resource manager
		 *  test it.
		 */
		request_resource(res, &code_resource);
		request_resource(res, &data_resource);
		request_resource(res, &bss_resource);
	}
}

#ifdef CONFIG_SMP
static void __init prefill_possible_map(void)
{
	int i, possible;

	possible = num_processors + disabled_cpus;
	if (possible > nr_cpu_ids)
		possible = nr_cpu_ids;

	pr_info("SMP: Allowing %d CPUs, %d hotplug CPUs\n",
			possible, max((possible - num_processors), 0));

	for (i = 0; i < possible; i++)
		set_cpu_possible(i, true);
	for (; i < NR_CPUS; i++)
		set_cpu_possible(i, false);

	nr_cpu_ids = possible;
}
#else
static inline void prefill_possible_map(void) {}
#endif
#ifdef CONFIG_EARLY_PRINTK
extern void setup_early_printk(void);
#endif

void __init setup_arch(char **cmdline_p)
{
	cpu_probe();
	early_init();

#ifdef CONFIG_EARLY_PRINTK
	setup_early_printk();
#endif
	bootcmdline_init(cmdline_p);

	init_initrd();
	platform_init();
	finalize_initrd();
	cpu_report();

	arch_mem_init(cmdline_p);

	resource_init();
#ifdef CONFIG_SMP
	plat_smp_setup();
#endif
	prefill_possible_map();

	cpu_cache_init();
	paging_init();
	boot_cpu_trap_init();
}
