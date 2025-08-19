/*
 * bootstub 32 bit entry setting routings
 *
 * Copyright (C) 2008-2010 Intel Corporation.
 * Author: Alek Du <alek.du@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "types.h"
#include "bootstub.h"
#include "bootparam.h"
#include "mb.h"
#include "sfi.h"
#include "simplefb.h"

#include <stdint.h>
#include <stddef.h>

#define FATAL_HANG()  { asm("cli"); while (1) { asm("nop"); } }

static memory_map_t mb_mmap[E820MAX];
u32 mb_magic, mb_info;

// The bootloader expects the structure of boot_img_hdr with header
// version 0 to be as follows:
struct boot_img_hdr {
    // Must be BOOT_MAGIC.
    uint8_t magic[BOOT_MAGIC_SIZE];
    uint32_t kernel_size; /* size in bytes */
    uint32_t kernel_addr; /* physical load addr */
    uint32_t ramdisk_size; /* size in bytes */
    uint32_t ramdisk_addr; /* physical load addr */
    uint32_t second_size; /* size in bytes */
    uint32_t second_addr; /* physical load addr */
    uint32_t tags_addr; /* physical addr for kernel tags (if required) */
    uint32_t page_size; /* flash page size we assume */
    // Version of the boot image header.
    uint32_t header_version;
    // Operating system version and security patch level.
    // For version "A.B.C" and patch level "Y-M-D":
    //   (7 bits for each of A, B, C; 7 bits for (Y-2000), 4 bits for M)
    //   os_version = A[31:25] B[24:18] C[17:11] (Y-2000)[10:4] M[3:0]
    uint32_t os_version;
#if __cplusplus
    void SetOsVersion(unsigned major, unsigned minor, unsigned patch) {
        os_version &= ((1 << 11) - 1);
        os_version |= (((major & 0x7f) << 25) | ((minor & 0x7f) << 18) | ((patch & 0x7f) << 11));
    }
    void SetOsPatchLevel(unsigned year, unsigned month) {
        os_version &= ~((1 << 11) - 1);
        os_version |= (((year - 2000) & 0x7f) << 4) | ((month & 0xf) << 0);
    }
#endif
    uint8_t name[BOOT_NAME_SIZE]; /* asciiz product name */
    uint8_t cmdline[BOOT_ARGS_SIZE];
    uint32_t id[8]; /* timestamp / checksum / sha1 / etc */
    // Supplemental command line data; kept here to maintain
    // binary compatibility with older versions of mkbootimg.
    uint8_t extra_cmdline[BOOT_EXTRA_ARGS_SIZE];
} __attribute__((packed));

struct gdt_ptr {
        u16 len;
        u32 ptr;
} __attribute__((packed));

static void *memcpy(void *dest, const void *src, size_t count)
{
        char *tmp = dest;
        const char *s = src;
	size_t _count = count / 4;

	while (_count--) {
		*(long *)tmp = *(long *)s;
		tmp += 4;
		s += 4;
	}
	count %= 4;
        while (count--)
                *tmp++ = *s++;
        return dest;
}

static void *memset(void *s, unsigned char c, size_t count)
{
        char *xs = s;
	size_t _count = count / 4;
	unsigned long  _c = c << 24 | c << 16 | c << 8 | c;

	while (_count--) {
		*(long *)xs = _c;
		xs += 4;
	}
	count %= 4;
        while (count--)
                *xs++ = c;
        return s;
}

static size_t strnlen(const char *s, size_t maxlen)
{
        const char *es = s;
        while (*es && maxlen) {
                es++;
                maxlen--;
        }

        return (es - s);
}

int strncmp(const char *cs, const char *ct, size_t count)
{
	unsigned char c1, c2;

	while (count) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
		count--;
	}
	return 0;
}

static inline int is_image_aosp(unsigned char *magic)
{
	return !strncmp((char *)magic, (char *)BOOT_MAGIC, sizeof(BOOT_MAGIC)-1);
}

static void setup_boot_params(struct boot_params *bp, struct setup_header *sh)
{
	bp->screen_info.orig_video_mode = 0;
	bp->screen_info.orig_video_lines = 0;
	bp->screen_info.orig_video_cols = 0;
	bp->alt_mem_k = 128*1024; // hard coded 128M mem here, since SFI will override it
	memcpy(&bp->hdr, sh, sizeof (struct setup_header));
	bp->hdr.type_of_loader = 0xff; //bootstub is unknown bootloader for kernel :)
	bp->hdr.hardware_subarch = X86_SUBARCH_MRST;
}

static u32 bzImage_setup(struct boot_params *bp)
{
	void *cmdline = (void *)BOOT_CMDLINE_OFFSET;
	struct boot_img_hdr *aosp = (struct boot_img_hdr *)AOSP_HEADER_ADDRESS;
	size_t cmdline_len, extra_cmdline_len;
	u8 *initramfs, *ptr;
	u32 bzimage_size;

	if (is_image_aosp(aosp->magic)) {
		ptr = (u8*)aosp->kernel_addr;
		cmdline_len = strnlen((const char *)aosp->cmdline, sizeof(aosp->cmdline));
		extra_cmdline_len = strnlen((const char *)aosp->extra_cmdline, sizeof(aosp->extra_cmdline));

		/*
		* Copy the command + extra command line to be after bootparams
		* so that it won't be overwritten by the kernel executable.
		*/
		memset(cmdline, 0, sizeof(aosp->cmdline) + sizeof(aosp->extra_cmdline));
		memcpy(cmdline, (const void *)aosp->cmdline, cmdline_len);
		memcpy(cmdline + cmdline_len, (const void *)aosp->extra_cmdline, extra_cmdline_len);

		bp->hdr.ramdisk_size = aosp->ramdisk_size;

		initramfs = (u8 *)aosp->ramdisk_addr;
		bzimage_size = aosp->kernel_size;
	} else {
		ptr = (u8*)BZIMAGE_OFFSET;
		cmdline_len = strnlen((const char *)CMDLINE_OFFSET, CMDLINE_SIZE);
		/*
		 * Copy the command line to be after bootparams so that it won't be
		 * overwritten by the kernel executable.
		 */
		memset(cmdline, 0, CMDLINE_SIZE);
		memcpy(cmdline, (const void *)CMDLINE_OFFSET, cmdline_len);

		bp->hdr.ramdisk_size = *(u32 *)INITRD_SIZE_OFFSET;

		initramfs = (u8 *)BZIMAGE_OFFSET + *(u32 *)BZIMAGE_SIZE_OFFSET;
		bzimage_size = *(u32 *)BZIMAGE_SIZE_OFFSET;
	}

	bp->hdr.cmd_line_ptr = BOOT_CMDLINE_OFFSET;
	bp->hdr.cmdline_size = cmdline_len;
#ifndef BUILD_RAMDUMP
	bp->hdr.ramdisk_image = (bp->alt_mem_k*1024 - bp->hdr.ramdisk_size) & 0xFFFFF000;

	if (!bzimage_size) {
		npf_pprintf(&simplefb_putc, NULL, "Nothing to boot!\n");
		FATAL_HANG();
	}

	if (bp->hdr.ramdisk_size) {
		npf_pprintf(&simplefb_putc, NULL, "Relocating initramfs to high memory ...\n");
		memcpy((u8*)bp->hdr.ramdisk_image, initramfs, bp->hdr.ramdisk_size);
	} else {
		npf_pprintf(&simplefb_putc, NULL, "Booting Linux without initramfs ...\n");
	}
#else
	bp->hdr.ramdisk_image = (u32) initramfs;
#endif

	while (1){
		if (*(u32 *)ptr == SETUP_SIGNATURE && *(u32 *)(ptr+4) == 0)
			break;
		ptr++;
	}
	ptr+=4;
	return (((unsigned int)ptr+511)/512)*512;
}

static void setup_gdt(void)
{
        static const u64 boot_gdt[] __attribute__((aligned(16))) = {
                /* CS: code, read/execute, 4 GB, base 0 */
                [GDT_ENTRY_BOOT_CS] = GDT_ENTRY(0xc09b, 0, 0xfffff),
                /* DS: data, read/write, 4 GB, base 0 */
                [GDT_ENTRY_BOOT_DS] = GDT_ENTRY(0xc093, 0, 0xfffff),
        };
        static struct gdt_ptr gdt;

        gdt.len = sizeof(boot_gdt)-1;
        gdt.ptr = (u32)&boot_gdt;

        asm volatile("lgdtl %0" : : "m" (gdt));
}

static void setup_idt(void)
{
        static const struct gdt_ptr null_idt = {0, 0};
        asm volatile("lidtl %0" : : "m" (null_idt));
}

static u32 multiboot_setup(void)
{
	u32 *header_magic, *mb_image, i;
	u32 mb_size;
	struct boot_img_hdr *aosp = (struct boot_img_hdr *)AOSP_HEADER_ADDRESS;
	static module_t modules[2];
	multiboot_header_t *mb_header;
	static multiboot_info_t mb = {
		.flags = MBI_CMDLINE | MBI_MODULES | MBI_MEMMAP | MBI_FB,
		.mmap_addr = (u32)mb_mmap,
		.mods_count = 2,
		.mods_addr = (u32)modules,
	};

	if (is_image_aosp(aosp->magic)) {
		mb.cmdline = (u32)aosp->cmdline;
		mb_size = aosp->kernel_size + aosp->ramdisk_size;
		mb_image = (u32 *)aosp->kernel_addr;
	} else {
		mb.cmdline = CMDLINE_OFFSET;
		mb_size =  *(u32 *)BZIMAGE_SIZE_OFFSET + *(u32 *)INITRD_SIZE_OFFSET;
		mb_image = (u32 *)BZIMAGE_OFFSET;
	}

	if (mb_size == 0)
		return 0;

	/* the multiboot signature should be located in the first 8192 bytes */
	for (header_magic = mb_image; header_magic < mb_image + 2048; header_magic++)
		if (*header_magic == MULTIBOOT_HEADER_MAGIC)
			break;

	/* Fallback to bzImage setup if this isn't a Multiboot image */
	if (*header_magic != MULTIBOOT_HEADER_MAGIC)
		return 0;

	mb_header = (multiboot_header_t *)header_magic;

	if (mb_header->checksum + (mb_header->magic + mb_header->flags) != 0) {
		npf_pprintf(&simplefb_putc, NULL, "Invalid Multiboot image! Expected checksum 0x%x, got 0x%x \n",
			   -mb_header->checksum, (mb_header->magic + mb_header->flags));
		FATAL_HANG();
	}

	npf_pprintf(&simplefb_putc, NULL, "Multiboot signature found! Image entry address is 0x%x\n", mb_header->entry_addr);

	/* fill in the multiboot module information: Multiboot image and initial ramdisk */
	modules[0].mod_start = mb_header->load_addr;
	modules[0].mod_end = mb_header->load_addr +
	((is_image_aosp(aosp->magic)) ? aosp->kernel_size : *(u32 *)BZIMAGE_SIZE_OFFSET);
	modules[0].string = 0;

	modules[1].mod_start =
	(is_image_aosp(aosp->magic)) ? aosp->ramdisk_addr : (BZIMAGE_OFFSET + *(u32 *)BZIMAGE_SIZE_OFFSET);
	modules[1].mod_end = modules[1].mod_start +
	((is_image_aosp(aosp->magic)) ? aosp->ramdisk_size : *(u32 *)INITRD_SIZE_OFFSET);
	modules[1].string = 0;

	for(i = 0; i < E820MAX; i++)
		if (!mb_mmap[i].size)
			break;
	mb.mmap_length = i * sizeof(memory_map_t);

	/* relocate Multiboot image to start address */
	memcpy((u8 *)mb_header->load_addr, mb_image, mb_size);
	mb_info = (u32)&mb;
	mb_magic = MULTIBOOT_BOOTLOADER_MAGIC;

	return mb_header->entry_addr;
}

int bootstub(void)
{
	u32 jmp;
	struct boot_img_hdr *aosp = (struct boot_img_hdr *)AOSP_HEADER_ADDRESS;
	struct boot_params *bp = (struct boot_params *)BOOT_PARAMS_OFFSET;
	struct setup_header *sh;

	if (is_image_aosp(aosp->magic)) {
		sh = (struct setup_header *)((unsigned  int)aosp->kernel_addr + \
		                             (unsigned  int)offsetof(struct boot_params,hdr));
	} else
		sh = (struct setup_header *)SETUP_HEADER_OFFSET;

	setup_idt();
	setup_gdt();
	simplefb_init();
	npf_pprintf(&simplefb_putc, NULL, "Bootstub Version: 1.4 ...\n");

	memset(bp, 0, sizeof (struct boot_params));
	sfi_setup_mmap(bp, mb_mmap);
	setup_boot_params(bp, sh);

	jmp = multiboot_setup();
	if (!jmp) {
		npf_pprintf(&simplefb_putc, NULL, "Using bzImage to boot\n");
		jmp = bzImage_setup(bp);
	} else
		npf_pprintf(&simplefb_putc, NULL, "Using multiboot image to boot\n");

	npf_pprintf(&simplefb_putc, NULL, "Jump to kernel 32bit entry\n");
	return jmp;
}
