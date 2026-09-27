// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 *  Copyright (C) 2001 Rusty Russell.
 *  Copyright (C) 2003, 2004 Ralf Baechle (ralf@linux-mips.org)
 *  Copyright (C) 2005 Thiemo Seufer
 */

#undef DEBUG

#include <linux/extable.h>
#include <linux/moduleloader.h>
#include <linux/elf.h>
#include <linux/mm.h>
#include <linux/numa.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/jump_label.h>

#include <asm/pgtable.h>	/* MODULE_START */

struct mips_hi16 {
	struct mips_hi16 *next;
	Elf_Addr *addr;
	Elf_Addr value;
};

static LIST_HEAD(dbe_list);
static DEFINE_SPINLOCK(dbe_lock);

/*
 * Get the potential max trampolines size required of the init and
 * non-init sections. Only used if we cannot find enough contiguous
 * physically mapped memory to put the module into.
 */
static unsigned int
get_plt_size(const Elf_Ehdr *hdr, const Elf_Shdr *sechdrs,
             const char *secstrings, unsigned int symindex, bool is_init)
{
	unsigned long ret = 0;
	unsigned int i, j;
	Elf_Sym *syms;

	/* Everything marked ALLOC (this includes the exported symbols) */
	for (i = 1; i < hdr->e_shnum; ++i) {
		unsigned int info = sechdrs[i].sh_info;

		if (sechdrs[i].sh_type != SHT_REL
		    && sechdrs[i].sh_type != SHT_RELA)
			continue;

		/* Not a valid relocation section? */
		if (info >= hdr->e_shnum)
			continue;

		/* Don't bother with non-allocated sections */
		if (!(sechdrs[info].sh_flags & SHF_ALLOC))
			continue;

		/* If it's called *.init*, and we're not init, we're
                   not interested */
		if ((strstr(secstrings + sechdrs[i].sh_name, ".init") != 0)
		    != is_init)
			continue;

		syms = (Elf_Sym *) sechdrs[symindex].sh_addr;
		if (sechdrs[i].sh_type == SHT_REL) {
			Elf_Mips_Rel *rel = (void *) sechdrs[i].sh_addr;
			unsigned int size = sechdrs[i].sh_size / sizeof(*rel);

			for (j = 0; j < size; ++j) {
				Elf_Sym *sym;

				if (ELF_MIPS_R_TYPE(rel[j]) != R_MIPS_26)
					continue;

				sym = syms + ELF_MIPS_R_SYM(rel[j]);
				if (!is_init && sym->st_shndx != SHN_UNDEF)
					continue;

				ret += 4 * sizeof(int);
			}
		} else {
			Elf_Mips_Rela *rela = (void *) sechdrs[i].sh_addr;
			unsigned int size = sechdrs[i].sh_size / sizeof(*rela);

			for (j = 0; j < size; ++j) {
				Elf_Sym *sym;

				if (ELF_MIPS_R_TYPE(rela[j]) != R_MIPS_26)
					continue;

				sym = syms + ELF_MIPS_R_SYM(rela[j]);
				if (!is_init && sym->st_shndx != SHN_UNDEF)
					continue;

				ret += 4 * sizeof(int);
			}
		}
	}

	return ret;
}

#ifndef MODULE_START
static void *alloc_phys(unsigned long size)
{
	unsigned order;
	struct page *page;
	struct page *p;

	size = PAGE_ALIGN(size);
	order = get_order(size);

	page = alloc_pages(GFP_KERNEL | __GFP_NORETRY | __GFP_NOWARN |
			__GFP_THISNODE, order);
	if (!page)
		return NULL;

	split_page(page, order);

	/* mark all pages except for the last one */
	for (p = page; p + 1 < page + (size >> PAGE_SHIFT); ++p)
		set_bit(PG_owner_priv_1, &p->flags);

	for (p = page + (size >> PAGE_SHIFT); p < page + (1 << order); ++p)
		__free_page(p);

	return page_address(page);
}
#endif

static void free_phys(void *ptr)
{
	struct page *page;
	bool free;

	page = virt_to_page(ptr);
	do {
		free = test_and_clear_bit(PG_owner_priv_1, &page->flags);
		__free_page(page);
		page++;
	} while (free);
}


void *module_alloc(unsigned long size)
{
#ifdef MODULE_START
	return __vmalloc_node_range(size, 1, MODULE_START, MODULE_END,
				GFP_KERNEL, PAGE_KERNEL, 0, NUMA_NO_NODE,
				__builtin_return_address(0));
#else
	void *ptr;

	if (size == 0)
		return NULL;

	ptr = alloc_phys(size);

	/* If we failed to allocate physically contiguous memory,
	 * fall back to regular vmalloc. The module loader code will
	 * create jump tables to handle long jumps */
	if (!ptr)
		return vmalloc(size);

	return ptr;
#endif
}

static inline bool is_phys_addr(void *ptr)
{
#ifdef CONFIG_64BIT
	return (KSEGX((unsigned long)ptr) == CKSEG0);
#else
	return (KSEGX(ptr) == KSEG0);
#endif
}

/* Free memory returned from module_alloc */
void module_memfree(void *module_region)
{
	if (is_phys_addr(module_region))
		free_phys(module_region);
	else
		vfree(module_region);
}

static void *__module_alloc(int size, bool phys)
{
	void *ptr;

	if (phys)
		ptr = kmalloc(size, GFP_KERNEL);
	else
		ptr = vmalloc(size);
	return ptr;
}

static void __module_free(void *ptr)
{
	if (is_phys_addr(ptr))
		kfree(ptr);
	else
		vfree(ptr);
}

int module_frob_arch_sections(Elf_Ehdr *hdr, Elf_Shdr *sechdrs,
			      char *secstrings, struct module *mod)
{
	unsigned int symindex = 0;
	unsigned int core_size, init_size;
	int i;

	mod->arch.phys_plt_offset = 0;
	mod->arch.virt_plt_offset = 0;
	mod->arch.phys_plt_tbl = NULL;
	mod->arch.virt_plt_tbl = NULL;

	if (IS_ENABLED(CONFIG_64BIT))
		return 0;

	for (i = 1; i < hdr->e_shnum; i++)
		if (sechdrs[i].sh_type == SHT_SYMTAB)
			symindex = i;

	core_size = get_plt_size(hdr, sechdrs, secstrings, symindex, false);
	init_size = get_plt_size(hdr, sechdrs, secstrings, symindex, true);

	if ((core_size + init_size) == 0)
		return 0;

	mod->arch.phys_plt_tbl = __module_alloc(core_size + init_size, 1);
	if (!mod->arch.phys_plt_tbl)
		return -ENOMEM;

	mod->arch.virt_plt_tbl = __module_alloc(core_size + init_size, 0);
	if (!mod->arch.virt_plt_tbl) {
		__module_free(mod->arch.phys_plt_tbl);
		mod->arch.phys_plt_tbl = NULL;
		return -ENOMEM;
	}

	return 0;
}

static int apply_r_mips_none(struct module *me, u32 *location,
			     u32 base, Elf_Addr v, bool rela)
{
	return 0;
}

static int apply_r_mips_32(struct module *me, u32 *location,
			   u32 base, Elf_Addr v, bool rela)
{
	*location = base + v;

	return 0;
}

static Elf_Addr add_plt_entry_to(unsigned *plt_offset,
				 void *start, Elf_Addr v)
{
	unsigned *tramp = start + *plt_offset;
	*plt_offset += 4 * sizeof(int);

	/* adjust carry for addiu */
	if (v & 0x00008000)
		v += 0x10000;

	tramp[0] = 0x3c190000 | (v >> 16);      /* lui t9, hi16 */
	tramp[1] = 0x27390000 | (v & 0xffff);   /* addiu t9, t9, lo16 */
	tramp[2] = 0x03200008;                  /* jr t9 */
	tramp[3] = 0x00000000;                  /* nop */

	return (Elf_Addr) tramp;
}

static Elf_Addr add_plt_entry(struct module *me, void *location, Elf_Addr v)
{
	if (is_phys_addr(location))
		return add_plt_entry_to(&me->arch.phys_plt_offset,
				me->arch.phys_plt_tbl, v);
	else
		return add_plt_entry_to(&me->arch.virt_plt_offset,
				me->arch.virt_plt_tbl, v);

}

static int apply_r_mips_26(struct module *me, u32 *location,
			   u32 base, Elf_Addr v, bool rela)
{
	u32 ofs = base & 0x03ffffff;

	if (v % 4) {
		pr_err("module %s: dangerous R_MIPS_26 relocation\n",
		       me->name);
		return -ENOEXEC;
	}

	if ((v & 0xf0000000) != (((unsigned long)location + 4) & 0xf0000000)) {
		v = add_plt_entry(me, location, v + (ofs << 2));
		if (!v) {
			pr_err("module %s: relocation overflow\n",
			       me->name);
			return -ENOEXEC;
		}
		ofs = 0;
	}

	*location = (*location & ~0x03ffffff) |
		    ((ofs + (v >> 2)) & 0x03ffffff);

	return 0;
}

static int apply_r_mips_hi16(struct module *me, u32 *location,
			     u32 base, Elf_Addr v, bool rela)
{
	struct mips_hi16 *n;

	if (rela) {
		*location = (*location & 0xffff0000) |
			    ((((long long) v + 0x8000LL) >> 16) & 0xffff);
		return 0;
	}

	/*
	 * We cannot relocate this one now because we don't know the value of
	 * the carry we need to add.  Save the information, and let LO16 do the
	 * actual relocation.
	 */
	n = kmalloc(sizeof *n, GFP_KERNEL);
	if (!n)
		return -ENOMEM;

	n->addr = (Elf_Addr *)location;
	n->value = v;
	n->next = me->arch.r_mips_hi16_list;
	me->arch.r_mips_hi16_list = n;

	return 0;
}

static void free_relocation_chain(struct mips_hi16 *l)
{
	struct mips_hi16 *next;

	while (l) {
		next = l->next;
		kfree(l);
		l = next;
	}
}

static int apply_r_mips_lo16(struct module *me, u32 *location,
			     u32 base, Elf_Addr v, bool rela)
{
	unsigned long insnlo = base;
	struct mips_hi16 *l;
	Elf_Addr val, vallo;

	if (rela) {
		*location = (*location & 0xffff0000) | (v & 0xffff);
		return 0;
	}

	/* Sign extend the addend we extract from the lo insn.	*/
	vallo = ((insnlo & 0xffff) ^ 0x8000) - 0x8000;

	if (me->arch.r_mips_hi16_list != NULL) {
		l = me->arch.r_mips_hi16_list;
		while (l != NULL) {
			struct mips_hi16 *next;
			unsigned long insn;

			/*
			 * The value for the HI16 had best be the same.
			 */
			if (v != l->value)
				goto out_danger;

			/*
			 * Do the HI16 relocation.  Note that we actually don't
			 * need to know anything about the LO16 itself, except
			 * where to find the low 16 bits of the addend needed
			 * by the LO16.
			 */
			insn = *l->addr;
			val = ((insn & 0xffff) << 16) + vallo;
			val += v;

			/*
			 * Account for the sign extension that will happen in
			 * the low bits.
			 */
			val = ((val >> 16) + ((val & 0x8000) != 0)) & 0xffff;

			insn = (insn & ~0xffff) | val;
			*l->addr = insn;

			next = l->next;
			kfree(l);
			l = next;
		}

		me->arch.r_mips_hi16_list = NULL;
	}

	/*
	 * Ok, we're done with the HI16 relocs.	 Now deal with the LO16.
	 */
	val = v + vallo;
	insnlo = (insnlo & ~0xffff) | (val & 0xffff);
	*location = insnlo;

	return 0;

out_danger:
	free_relocation_chain(l);
	me->arch.r_mips_hi16_list = NULL;

	pr_err("module %s: dangerous R_MIPS_LO16 relocation\n", me->name);

	return -ENOEXEC;
}

static int apply_r_mips_pc(struct module *me, u32 *location, u32 base,
			   Elf_Addr v, unsigned int bits)
{
	unsigned long mask = GENMASK(bits - 1, 0);
	unsigned long se_bits;
	long offset;

	if (v % 4) {
		pr_err("module %s: dangerous R_MIPS_PC%u relocation\n",
		       me->name, bits);
		return -ENOEXEC;
	}

	/* retrieve & sign extend implicit addend if any */
	offset = base & mask;
	offset |= (offset & BIT(bits - 1)) ? ~mask : 0;

	offset += ((long)v - (long)location) >> 2;

	/* check the sign bit onwards are identical - ie. we didn't overflow */
	se_bits = (offset & BIT(bits - 1)) ? ~0ul : 0;
	if ((offset & ~mask) != (se_bits & ~mask)) {
		pr_err("module %s: relocation overflow\n", me->name);
		return -ENOEXEC;
	}

	*location = (*location & ~mask) | (offset & mask);

	return 0;
}

static int apply_r_mips_pc16(struct module *me, u32 *location,
			     u32 base, Elf_Addr v, bool rela)
{
	return apply_r_mips_pc(me, location, base, v, 16);
}

static int apply_r_mips_pc21(struct module *me, u32 *location,
			     u32 base, Elf_Addr v, bool rela)
{
	return apply_r_mips_pc(me, location, base, v, 21);
}

static int apply_r_mips_pc26(struct module *me, u32 *location,
			     u32 base, Elf_Addr v, bool rela)
{
	return apply_r_mips_pc(me, location, base, v, 26);
}

static int apply_r_mips_64(struct module *me, u32 *location,
			   u32 base, Elf_Addr v, bool rela)
{
	if (WARN_ON(!rela))
		return -EINVAL;

	*(Elf_Addr *)location = v;

	return 0;
}

static int apply_r_mips_higher(struct module *me, u32 *location,
			       u32 base, Elf_Addr v, bool rela)
{
	if (WARN_ON(!rela))
		return -EINVAL;

	*location = (*location & 0xffff0000) |
		    ((((long long)v + 0x80008000LL) >> 32) & 0xffff);

	return 0;
}

static int apply_r_mips_highest(struct module *me, u32 *location,
				u32 base, Elf_Addr v, bool rela)
{
	if (WARN_ON(!rela))
		return -EINVAL;

	*location = (*location & 0xffff0000) |
		    ((((long long)v + 0x800080008000LL) >> 48) & 0xffff);

	return 0;
}

/**
 * reloc_handler() - Apply a particular relocation to a module
 * @me: the module to apply the reloc to
 * @location: the address at which the reloc is to be applied
 * @base: the existing value at location for REL-style; 0 for RELA-style
 * @v: the value of the reloc, with addend for RELA-style
 *
 * Each implemented reloc_handler function applies a particular type of
 * relocation to the module @me. Relocs that may be found in either REL or RELA
 * variants can be handled by making use of the @base & @v parameters which are
 * set to values which abstract the difference away from the particular reloc
 * implementations.
 *
 * Return: 0 upon success, else -ERRNO
 */
typedef int (*reloc_handler)(struct module *me, u32 *location,
			     u32 base, Elf_Addr v, bool rela);

/* The handlers for known reloc types */
static reloc_handler reloc_handlers[] = {
	[R_MIPS_NONE]		= apply_r_mips_none,
	[R_MIPS_32]		= apply_r_mips_32,
	[R_MIPS_26]		= apply_r_mips_26,
	[R_MIPS_HI16]		= apply_r_mips_hi16,
	[R_MIPS_LO16]		= apply_r_mips_lo16,
	[R_MIPS_PC16]		= apply_r_mips_pc16,
	[R_MIPS_64]		= apply_r_mips_64,
	[R_MIPS_HIGHER]		= apply_r_mips_higher,
	[R_MIPS_HIGHEST]	= apply_r_mips_highest,
	[R_MIPS_PC21_S2]	= apply_r_mips_pc21,
	[R_MIPS_PC26_S2]	= apply_r_mips_pc26,
};

static int __apply_relocate(Elf_Shdr *sechdrs, const char *strtab,
			    unsigned int symindex, unsigned int relsec,
			    struct module *me, bool rela)
{
	union {
		Elf_Mips_Rel *rel;
		Elf_Mips_Rela *rela;
	} r;
	reloc_handler handler;
	Elf_Sym *sym;
	u32 *location, base;
	unsigned int i, type;
	Elf_Addr v;
	int err = 0;
	size_t reloc_sz;

	pr_debug("Applying relocate section %u to %u\n", relsec,
	       sechdrs[relsec].sh_info);

	r.rel = (void *)sechdrs[relsec].sh_addr;
	reloc_sz = rela ? sizeof(*r.rela) : sizeof(*r.rel);
	me->arch.r_mips_hi16_list = NULL;
	for (i = 0; i < sechdrs[relsec].sh_size / reloc_sz; i++) {
		/* This is where to make the change */
		location = (void *)sechdrs[sechdrs[relsec].sh_info].sh_addr
			+ r.rel->r_offset;
		/* This is the symbol it is referring to */
		sym = (Elf_Sym *)sechdrs[symindex].sh_addr
			+ ELF_MIPS_R_SYM(*r.rel);
		if (sym->st_value >= -MAX_ERRNO) {
			/* Ignore unresolved weak symbol */
			if (ELF_ST_BIND(sym->st_info) == STB_WEAK)
				continue;
			pr_warn("%s: Unknown symbol %s\n",
				me->name, strtab + sym->st_name);
			err = -ENOENT;
			goto out;
		}

		type = ELF_MIPS_R_TYPE(*r.rel);
		if (type < ARRAY_SIZE(reloc_handlers))
			handler = reloc_handlers[type];
		else
			handler = NULL;

		if (!handler) {
			pr_err("%s: Unknown relocation type %u\n",
			       me->name, type);
			err = -EINVAL;
			goto out;
		}

		if (rela) {
			v = sym->st_value + r.rela->r_addend;
			base = 0;
			r.rela = &r.rela[1];
		} else {
			v = sym->st_value;
			base = *location;
			r.rel = &r.rel[1];
		}

		err = handler(me, location, base, v, rela);
		if (err)
			goto out;
	}

out:
	/*
	 * Normally the hi16 list should be deallocated at this point. A
	 * malformed binary however could contain a series of R_MIPS_HI16
	 * relocations not followed by a R_MIPS_LO16 relocation, or if we hit
	 * an error processing a reloc we might have gotten here before
	 * reaching the R_MIPS_LO16. In either case, free up the list and
	 * return an error.
	 */
	if (me->arch.r_mips_hi16_list) {
		free_relocation_chain(me->arch.r_mips_hi16_list);
		me->arch.r_mips_hi16_list = NULL;
		err = err ?: -ENOEXEC;
	}

	return err;
}

int apply_relocate(Elf_Shdr *sechdrs, const char *strtab,
		   unsigned int symindex, unsigned int relsec,
		   struct module *me)
{
	return __apply_relocate(sechdrs, strtab, symindex, relsec, me, false);
}

#ifdef CONFIG_MODULES_USE_ELF_RELA
int apply_relocate_add(Elf_Shdr *sechdrs, const char *strtab,
		       unsigned int symindex, unsigned int relsec,
		       struct module *me)
{
	return __apply_relocate(sechdrs, strtab, symindex, relsec, me, true);
}
#endif /* CONFIG_MODULES_USE_ELF_RELA */

/* Given an address, look for it in the module exception tables. */
const struct exception_table_entry *search_module_dbetables(unsigned long addr)
{
	unsigned long flags;
	const struct exception_table_entry *e = NULL;
	struct mod_arch_specific *dbe;

	spin_lock_irqsave(&dbe_lock, flags);
	list_for_each_entry(dbe, &dbe_list, dbe_list) {
		e = search_extable(dbe->dbe_start,
				   dbe->dbe_end - dbe->dbe_start, addr);
		if (e)
			break;
	}
	spin_unlock_irqrestore(&dbe_lock, flags);

	/* Now, if we found one, we are running inside it now, hence
	   we cannot unload the module, hence no refcnt needed. */
	return e;
}

/* Put in dbe list if necessary. */
int module_finalize(const Elf_Ehdr *hdr,
		    const Elf_Shdr *sechdrs,
		    struct module *me)
{
	const Elf_Shdr *s;
	char *secstrings = (void *)hdr + sechdrs[hdr->e_shstrndx].sh_offset;

	/* Make jump label nops. */
	jump_label_apply_nops(me);

	INIT_LIST_HEAD(&me->arch.dbe_list);
	for (s = sechdrs; s < sechdrs + hdr->e_shnum; s++) {
		if (strcmp("__dbe_table", secstrings + s->sh_name) != 0)
			continue;
		me->arch.dbe_start = (void *)s->sh_addr;
		me->arch.dbe_end = (void *)s->sh_addr + s->sh_size;
		spin_lock_irq(&dbe_lock);
		list_add(&me->arch.dbe_list, &dbe_list);
		spin_unlock_irq(&dbe_lock);
	}

	/* Get rid of the fixup trampoline if we're running the module
	 * from physically mapped address space */
	if (me->arch.phys_plt_offset == 0) {
		__module_free(me->arch.phys_plt_tbl);
		me->arch.phys_plt_tbl = NULL;
	}
	if (me->arch.virt_plt_offset == 0) {
		__module_free(me->arch.virt_plt_tbl);
		me->arch.virt_plt_tbl = NULL;
	}

	return 0;
}

void module_arch_freeing_init(struct module *mod)
{
	if (mod->state == MODULE_STATE_LIVE)
		return;

	if (mod->arch.phys_plt_tbl) {
		__module_free(mod->arch.phys_plt_tbl);
		mod->arch.phys_plt_tbl = NULL;
	}
	if (mod->arch.virt_plt_tbl) {
		__module_free(mod->arch.virt_plt_tbl);
		mod->arch.virt_plt_tbl = NULL;
	}
}

void module_arch_cleanup(struct module *mod)
{
	spin_lock_irq(&dbe_lock);
	list_del(&mod->arch.dbe_list);
	spin_unlock_irq(&dbe_lock);
}
