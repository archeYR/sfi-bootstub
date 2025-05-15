CC=gcc -nostartfiles -Wall -Werror -Os -nodefaultlibs -fno-stack-protector -ffunction-sections -fdata-sections -m32 -ffreestanding -Wl,-m elf_i386 -Wl,-T 2ndbootloader.lds -Wl,--defsym=BOOTSTUB_ENTRY=0x10F00000 -Wl,--gc-sections -Wl,-no-pie -Wl,--no-warn-rwx-segments -o bootstub
OBJCOPY=objcopy -O binary -R .note -R .comment -S
all:
	$(CC) bootstub.c sfi.c simplefb.c printf.c head.S
	$(OBJCOPY) bootstub
