// SPDX-License-Identifier: GPL-2.0-only
/*
 * machine_kexec.c for kexec
 * Created by <nschichan@corp.free.fr> on Thu Oct 12 15:15:06 2006
 */
#include <linux/compiler.h>
#include <linux/kexec.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/libfdt.h>

#include <asm/bootinfo.h>
#include <asm/cacheflush.h>
#include <asm/page.h>
#include <linux/uaccess.h>
#include "machine_kexec.h"

static unsigned long reboot_code_buffer;

#ifdef CONFIG_SMP
static void (*relocated_kexec_smp_wait)(void *);

atomic_t kexec_ready_to_reboot = ATOMIC_INIT(0);
void (*_crash_smp_send_stop)(void) = NULL;
#endif

void (*_machine_kexec_shutdown)(void) = NULL;
void (*_machine_crash_shutdown)(struct pt_regs *regs) = NULL;

static void machine_kexec_print_args(void)
{
	unsigned long argc = (int)kexec_args[0];
	int i;

	pr_info("kexec_args[0] (argc): %lu\n", argc);
	pr_info("kexec_args[1] (argv): %p\n", (void *)kexec_args[1]);
	pr_info("kexec_args[2] (env ): %p\n", (void *)kexec_args[2]);
	pr_info("kexec_args[3] (desc): %p\n", (void *)kexec_args[3]);

	for (i = 0; i < argc; i++) {
		pr_info("kexec_argv[%d] = %p, %s\n",
				i, kexec_argv[i], kexec_argv[i]);
	}
}

static void machine_kexec_init_argv(struct kimage *image)
{
	void __user *buf = NULL;
	size_t bufsz;
	size_t size;
	int i;

	bufsz = 0;
	for (i = 0; i < image->nr_segments; i++) {
		struct kexec_segment *seg;

		seg = &image->segment[i];
		if (seg->bufsz < 6)
			continue;

		if (strncmp((char *) seg->buf, "kexec ", 6))
			continue;

		buf = seg->buf;
		bufsz = seg->bufsz;
		break;
	}

	if (!buf)
		return;

	size = KEXEC_COMMAND_LINE_SIZE;
	size = min(size, bufsz);
	if (size < bufsz)
		pr_warn("kexec command line truncated to %zd bytes\n", size);

	/* Copy to kernel space */
	if (copy_from_user(kexec_argv_buf, buf, size))
		pr_warn("kexec command line copy to kernel space failed\n");

	kexec_argv_buf[size - 1] = 0;
}

static void machine_kexec_parse_argv(struct kimage *image)
{
	char *reboot_code_buffer;
	int reloc_delta;
	char *ptr;
	int argc;
	int i;

	ptr = kexec_argv_buf;
	argc = 0;

	/*
	 * convert command line string to array of parameters
	 * (as bootloader does).
	 */
	while (ptr && *ptr && (KEXEC_MAX_ARGC > argc)) {
		if (*ptr == ' ') {
			*ptr++ = '\0';
			continue;
		}

		kexec_argv[argc++] = ptr;
		ptr = strchr(ptr, ' ');
	}

	if (!argc)
		return;

	kexec_args[0] = argc;
	kexec_args[1] = (unsigned long)kexec_argv;
	kexec_args[2] = 0;
	kexec_args[3] = 0;

	reboot_code_buffer = page_address(image->control_code_page);
	reloc_delta = reboot_code_buffer - (char *)kexec_relocate_new_kernel;

	kexec_args[1] += reloc_delta;
	for (i = 0; i < argc; i++)
		kexec_argv[i] += reloc_delta;
}

static void kexec_image_info(const struct kimage *kimage)
{
	unsigned long i;

	pr_debug("kexec kimage info:\n");
	pr_debug("  type:        %d\n", kimage->type);
	pr_debug("  start:       %lx\n", kimage->start);
	pr_debug("  head:        %lx\n", kimage->head);
	pr_debug("  nr_segments: %lu\n", kimage->nr_segments);

	for (i = 0; i < kimage->nr_segments; i++) {
		pr_debug("    segment[%lu]: %016lx - %016lx, 0x%lx bytes, %lu pages\n",
			i,
			kimage->segment[i].mem,
			kimage->segment[i].mem + kimage->segment[i].memsz,
			(unsigned long)kimage->segment[i].memsz,
			(unsigned long)kimage->segment[i].memsz /  PAGE_SIZE);
	}
}

#ifdef CONFIG_UHI_BOOT

static int uhi_machine_kexec_prepare(struct kimage *kimage)
{
	int i;

	/*
	 * In case DTB file is not passed to the new kernel, a flat device
	 * tree will be created by kexec tool. It holds modified command
	 * line for the new kernel.
	 */
	for (i = 0; i < kimage->nr_segments; i++) {
		struct fdt_header fdt;

		if (kimage->segment[i].memsz <= sizeof(fdt))
			continue;

		if (copy_from_user(&fdt, kimage->segment[i].buf, sizeof(fdt)))
			continue;

		if (fdt_check_header(&fdt))
			continue;

		kexec_args[0] = -2;
		kexec_args[1] = (unsigned long)
			phys_to_virt((unsigned long)kimage->segment[i].mem);
		break;
	}

	return 0;
}

int (*_machine_kexec_prepare)(struct kimage *) = uhi_machine_kexec_prepare;

#else

int (*_machine_kexec_prepare)(struct kimage *) = NULL;

#endif /* CONFIG_UHI_BOOT */

int
machine_kexec_prepare(struct kimage *kimage)
{
#ifdef CONFIG_SMP
	if (!kexec_nonboot_cpu_func())
		return -EINVAL;
#endif

	kexec_image_info(kimage);
	/*
	 * Whenever arguments passed from kexec-tools, Init the arguments as
	 * the original ones to try avoiding booting failure.
	 */

	kexec_args[0] = fw_arg0;
	kexec_args[1] = fw_arg1;
	kexec_args[2] = fw_arg2;
	kexec_args[3] = fw_arg3;

	machine_kexec_init_argv(kimage);
	machine_kexec_parse_argv(kimage);

	if (_machine_kexec_prepare)
		return _machine_kexec_prepare(kimage);

	return 0;
}

void
machine_kexec_cleanup(struct kimage *kimage)
{
}

#ifdef CONFIG_SMP
static void kexec_shutdown_secondary(void *param)
{
	int cpu = smp_processor_id();

	if (!cpu_online(cpu))
		return;

	/* We won't be sent IPIs any more. */
	set_cpu_online(cpu, false);

	local_irq_disable();
	while (!atomic_read(&kexec_ready_to_reboot))
		cpu_relax();

	kexec_reboot();

	/* NOTREACHED */
}
#endif

void
machine_shutdown(void)
{
	if (_machine_kexec_shutdown)
		_machine_kexec_shutdown();

#ifdef CONFIG_SMP
	smp_call_function(kexec_shutdown_secondary, NULL, 0);

	while (num_online_cpus() > 1) {
		cpu_relax();
		mdelay(1);
	}
#endif
}

void
machine_crash_shutdown(struct pt_regs *regs)
{
	if (_machine_crash_shutdown)
		_machine_crash_shutdown(regs);
	else
		default_machine_crash_shutdown(regs);
}

#ifdef CONFIG_SMP
void kexec_nonboot_cpu_jump(void)
{
	local_flush_icache_range((unsigned long)relocated_kexec_smp_wait,
				 reboot_code_buffer + KEXEC_RELOCATE_NEW_KERNEL_SIZE);

	relocated_kexec_smp_wait(NULL);
}
#endif

void kexec_reboot(void)
{
	void (*do_kexec)(void) __noreturn;

	/*
	 * We know we were online, and there will be no incoming IPIs at
	 * this point. Mark online again before rebooting so that the crash
	 * analysis tool will see us correctly.
	 */
	set_cpu_online(smp_processor_id(), true);

	/* Ensure remote CPUs observe that we're online before rebooting. */
	smp_mb__after_atomic();

#ifdef CONFIG_SMP
	if (smp_processor_id() > 0) {
		/*
		 * Instead of cpu_relax() or wait, this is needed for kexec
		 * smp reboot. Kdump usually doesn't require an smp new
		 * kernel, but kexec may do.
		 */
		kexec_nonboot_cpu();

		/* NOTREACHED */
	}
#endif

	/*
	 * Make sure we get correct instructions written by the
	 * machine_kexec() CPU.
	 */
	local_flush_icache_range(reboot_code_buffer,
				 reboot_code_buffer + KEXEC_RELOCATE_NEW_KERNEL_SIZE);

	do_kexec = (void *)reboot_code_buffer;
	do_kexec();
}

void
machine_kexec(struct kimage *image)
{
	unsigned long entry;
	unsigned long *ptr;

	reboot_code_buffer =
		(unsigned long)page_address(image->control_code_page);
	pr_info("reboot_code_buffer = %p\n", (void *)reboot_code_buffer);

	kexec_start_address =
		(unsigned long) phys_to_virt(image->start);
	pr_info("kexec_start_address = %p\n", (void *)kexec_start_address);

	if (image->type == KEXEC_TYPE_DEFAULT) {
		kexec_indirection_page =
			(unsigned long) phys_to_virt(image->head & PAGE_MASK);
	} else {
		kexec_indirection_page = (unsigned long)&image->head;
	}
	pr_info("kexec_indirection_page = %p\n", (void *)kexec_indirection_page);

	pr_info("Where is memcpy: %p\n", memcpy);
	pr_info("kexec_relocate_new_kernel = %p, kexec_relocate_new_kernel_end = %p\n",
		(void *)kexec_relocate_new_kernel, &kexec_relocate_new_kernel_end);
	pr_info("Copy %lu bytes from %p to %p\n", KEXEC_RELOCATE_NEW_KERNEL_SIZE,
		(void *)kexec_relocate_new_kernel, (void *)reboot_code_buffer);
	memcpy((void*)reboot_code_buffer, kexec_relocate_new_kernel,
	       KEXEC_RELOCATE_NEW_KERNEL_SIZE);

	pr_info("Before _print_args().\n");
	machine_kexec_print_args();
	pr_info("Before eval loop.\n");

	/*
	 * The generic kexec code builds a page list with physical
	 * addresses. they are directly accessible through KSEG0 (or
	 * CKSEG0 or XPHYS if on 64bit system), hence the
	 * phys_to_virt() call.
	 */
	for (ptr = &image->head; (entry = *ptr) && !(entry &IND_DONE);
	     ptr = (entry & IND_INDIRECTION) ?
	       phys_to_virt(entry & PAGE_MASK) : ptr + 1) {
		if (*ptr & IND_SOURCE || *ptr & IND_INDIRECTION ||
		    *ptr & IND_DESTINATION)
			*ptr = (unsigned long) phys_to_virt(*ptr);
	}

	/* Mark offline BEFORE disabling local irq. */
	set_cpu_online(smp_processor_id(), false);

	/*
	 * we do not want to be bothered.
	 */
	local_irq_disable();

	printk("Will call new kernel at %08lx\n", image->start);
	printk("Bye ...\n");
	/* Make reboot code buffer available to the boot CPU. */
	__flush_cache_all();
#ifdef CONFIG_SMP
	/* All secondary cpus now may jump to kexec_wait cycle */
	relocated_kexec_smp_wait = reboot_code_buffer +
		(void *)(kexec_smp_wait - kexec_relocate_new_kernel);
	smp_wmb();
	atomic_set(&kexec_ready_to_reboot, 1);
#endif
	kexec_reboot();
}
