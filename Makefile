CC=gcc -nostartfiles -Wall -Werror -Oz -nodefaultlibs -fno-stack-protector -ffunction-sections -fdata-sections -m32 -ffreestanding -Wl,-m elf_i386 -Wl,-T 2ndbootloader.lds -Wl,--defsym=BOOTSTUB_ENTRY=0x01101000 -Wl,--gc-sections -Wl,-no-pie -Wl,--no-warn-rwx-segments -o bootstub -DGIT_SHA=\"$(shell git rev-parse HEAD | cut -c 1-10)\"
OBJCOPY=objcopy -O binary -R .note -R .comment -S
all:
	$(CC) bootstub.c sfi.c simplefb.c head.S
	$(OBJCOPY) bootstub
