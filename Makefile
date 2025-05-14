CC=gcc -nostartfiles -Wall -Werror -O1 -m32 -ffreestanding -Wl,-m elf_i386 -Wl,-T 2ndbootloader.lds -Wl,--defsym=BOOTSTUB_ENTRY=0x10F00000 -Wl,-no-pie -o bootstub
OBJCOPY=objcopy -O binary -R .note -R .comment -S
all:
	$(CC) bootstub.c imr_toc.c sfi.c spi-uart.c ssp-uart.c head.S e820_bios.S
	$(OBJCOPY) bootstub
