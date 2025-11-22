## SFI-Bootstub

Bootstub is a Linux bootloader that is used on various Intel MID devices. This fork focuses on bringing the compatibility with newer Linux kernels as well as standard Multiboot images to the SFI devices. It also supports logging to device's linear framebuffer for feedback and debugging. Note that for generating a boot image with SFI-Bootstub, you have to use modified mkboot tool from https://github.com/archeYR/intel-ifwi-study.

### Building:

For Moorestown/Medfield/Clover Trail+
```
PLATFORM=MOORESTOWN make
```
For Merrifield/Moorefield
```
PLATFORM=MOOREFIELD make
```

### Generating boot images with new Bootstub:

For devices with Intel boot image format use `mkboot.sh` from https://github.com/archeYR/intel-ifwi-study

For devices with AOSP boot image format:

- Append a <a href="https://github.com/viler-int10h/vga-text-mode-fonts/blob/master/FONTS/PC-IBM/VGA8.F16">font</a> to Bootstub image:
```
cat VGA8.F16 >> bootstub
```
- Generate a boot image with new Bootstub using `mkbootimg` (example for Asus Z00A):
```
mkbootimg --base 0x10000000 --kernel_offset 0x00008000 --ramdisk_offset 0x01000000 --tags_offset 0x00000100 --pagesize 2048 --second_offset 0xf00000 --kernel kernel --ramdisk ramdisk --second bootstub --cmdline <Z00A's cmdline> --output boot.img
```
