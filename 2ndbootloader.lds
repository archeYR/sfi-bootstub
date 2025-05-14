OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

MEMORY
{
    ROM (rx) : ORIGIN = BOOTSTUB_ENTRY, LENGTH = 8k
    RAM (wx) : org = BOOTSTUB_ENTRY+8k, len = 4k
}

SECTIONS
{
	. = BOOTSTUB_ENTRY;		/* bootstub entry in DRAM */
	.text.head : {
		_head = . ;
		*(.text.head)
		_ehead = . ;
	} > ROM
	.text :	{
		_text = .; 	/* Text */
		*(.text)
		*(.text.*)
		_etext = . ;
	} > ROM
	.rodata : {
		_rodata = . ;
		*(.rodata)	 /* read-only data */
		*(.rodata.*)
		_erodata = . ;
	} > ROM
	.data :	{
		_data = . ;
		*(.data)
		*(.data.*)
		_edata = . ;
	} > RAM
	.bss : {
		_bss = . ;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		_end = . ;
	} > RAM
}
