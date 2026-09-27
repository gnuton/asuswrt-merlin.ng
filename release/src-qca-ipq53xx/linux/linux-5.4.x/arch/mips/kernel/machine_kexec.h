#ifndef _MACHINE_KEXEC_H
#define _MACHINE_KEXEC_H

#ifndef __ASSEMBLY__
extern const unsigned char kexec_relocate_new_kernel[];
extern unsigned long kexec_relocate_new_kernel_end;
extern unsigned long kexec_start_address;
extern unsigned long kexec_indirection_page;

extern char kexec_argv_buf[];
extern char *kexec_argv[];

#define KEXEC_RELOCATE_NEW_KERNEL_SIZE	((unsigned long)&kexec_relocate_new_kernel_end - (unsigned long)kexec_relocate_new_kernel)
#endif /* !__ASSEMBLY__ */

#define KEXEC_COMMAND_LINE_SIZE		256
#define KEXEC_ARGV_SIZE			(KEXEC_COMMAND_LINE_SIZE / 16)
#define KEXEC_MAX_ARGC			(KEXEC_ARGV_SIZE / sizeof(long))

#endif
