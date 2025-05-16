/* define bootstub constrains here, like memory map etc.
 */

#ifndef _BOOT_STUB_HEAD
#define _BOOT_STUB_HEAD

#define BASE_ADDRESS		0x01100000

#define CMDLINE_OFFSET		BASE_ADDRESS
#define BZIMAGE_SIZE_OFFSET	(CMDLINE_OFFSET + CMDLINE_SIZE)
#define INITRD_SIZE_OFFSET	(BZIMAGE_SIZE_OFFSET + 4)

/* bootstub/android.mk definitions */
#define STACK_OFFSET		(BASE_ADDRESS + 0x1000)
#define CMDLINE_SIZE		0x400
#define AOSP_HEADER_ADDRESS	0x10007800

#define BOOTSTUB_OFFSET		STACK_OFFSET
#define FONT_OFFSET			(BASE_ADDRESS + 0x3000)

#define BZIMAGE_OFFSET		(BASE_ADDRESS + 0x4000)
#define MB_SIZE_OFFSET		BZIMAGE_SIZE_OFFSET

#define SETUP_HEADER_OFFSET (BZIMAGE_OFFSET + 0x1F1)
#define SETUP_HEADER_SIZE (0x0202 + *(unsigned char*)(0x0201+BZIMAGE_OFFSET))
#define BOOT_PARAMS_OFFSET 0x8000
#define BOOT_CMDLINE_OFFSET 0x10000
#define SETUP_SIGNATURE 0x5a5aaa55

#define GDT_ENTRY_BOOT_CS       2
#define __BOOT_CS               (GDT_ENTRY_BOOT_CS * 8)

#define GDT_ENTRY_BOOT_DS       (GDT_ENTRY_BOOT_CS + 1)
#define __BOOT_DS               (GDT_ENTRY_BOOT_DS * 8)

#ifdef __ASSEMBLER__
#define GDT_ENTRY(flags, base, limit)			\
	((((base)  & 0xff000000) << (56-24)) |	\
	 (((flags) & 0x0000f0ff) << 40) |	\
	 (((limit) & 0x000f0000) << (48-16)) |	\
	 (((base)  & 0x00ffffff) << 16) |	\
	 (((limit) & 0x0000ffff)))
#else
#define GDT_ENTRY(flags, base, limit)           \
        (((u64)(base & 0xff000000) << 32) |     \
         ((u64)flags << 40) |                   \
         ((u64)(limit & 0x00ff0000) << 32) |    \
         ((u64)(base & 0x00ffffff) << 16) |     \
         ((u64)(limit & 0x0000ffff)))
void bs_printk(const char *str);
void bs_print(const char *str, unsigned int value);
#endif

// boot_img_hdr_v0 struct
#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512
#define BOOT_EXTRA_ARGS_SIZE 1024

#endif
