#
# All Loongson based systems
#

obj-y += setup.o env.o reset.o irq.o mem.o \
	 dma.o rtc.o boardinfo.o

obj-$(CONFIG_SMP)	+= smp.o

obj-$(CONFIG_NUMA)	+= numa.o

obj-$(CONFIG_PCI_MSI)	+= msi.o
