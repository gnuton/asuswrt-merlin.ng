/* SPDX-License-Identifier: GPL-2.0 */
/* Host-only kernel API substitutes. These do not emulate switch hardware. */
#ifndef RTL8372_KERNEL_TEST_H
#define RTL8372_KERNEL_TEST_H
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm-generic/errno.h>
#include <uapi/asm-generic/ioctl.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint32_t __u32;
typedef int32_t __s32;
typedef uint16_t __u16;
typedef uint8_t __u8;
#define __aligned_u64 uint64_t __attribute__((aligned(8)))
#define BUILD_BUG_ON(x) _Static_assert(!(x), #x)
#define EXPORT_SYMBOL_GPL(x)
#define __user
#define __init
#define __exit
#define CAP_NET_ADMIN 12
#define VERIFY_WRITE 1
#define THIS_MODULE NULL
#define MISC_DYNAMIC_MINOR 255
#define no_llseek NULL
#define MODULE_LICENSE(x)
#define MODULE_DESCRIPTION(x)
#define module_init(x)
#define module_exit(x)
#define u64_to_user_ptr(x) ((void *)(uintptr_t)(x))
#define compat_ptr(x) ((void *)(uintptr_t)(uint32_t)(x))
static int test_privileged = 1, test_compat, test_copy_in_fail;
static int test_copy_out_fail, test_alloc_fail, test_locked, test_reads;
static int test_fail_read, test_not_initialized;

struct file { int unused; };
struct file_operations {
	void *owner;
	long (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);
	long (*compat_ioctl)(struct file *, unsigned int, unsigned long);
	void *llseek;
};
struct miscdevice {
	int minor;
	const char *name;
	const struct file_operations *fops;
	unsigned int mode;
};
static int capable(int capability)
{
	assert(capability == CAP_NET_ADMIN);
	return test_privileged;
}
static int is_compat_task(void) { return test_compat; }
static int access_ok(int type, const void *p, size_t size)
{
	assert(type == VERIFY_WRITE && size);
	return p != NULL;
}
static int copy_from_user(void *to, const void *from, size_t size)
{
	if (test_copy_in_fail)
		return 1;
	memcpy(to, from, size);
	return 0;
}
static int copy_to_user(void *to, const void *from, size_t size)
{
	if (test_copy_out_fail)
		return 1;
	memcpy(to, from, size);
	return 0;
}
static void *vzalloc(size_t size)
{
	return test_alloc_fail ? NULL : calloc(1, size);
}
static void vfree(void *p) { free(p); }
static void cond_resched(void) { assert(test_locked); }
static int misc_register(struct miscdevice *d)
{
	assert(d->mode == 0600);
	return 0;
}
static void misc_deregister(struct miscdevice *d) { assert(d->mode == 0600); }
#endif
