#include "types.h"

struct simplefb_data {
    u32    BaseAddress;
    u16    ScreenWidth;
    u16    ScreenHeight;
    u16    PixelsPerScanLine;
};

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16
#define TOP_BOTTOM_LINES 0

/* GMA device IDs */
#define DEV_GMA_MRST   0x41008086
#define DEV_GMA_MDFLD  0x01308086
#define DEV_GMA_CLTP   0x08c08086
#define DEV_GMA_MRFLD  0x11808086
#define DEV_GMA_MRFLD2 0x14808086
#define INTEL_DEV_MASK 0xfff0ffff

/*
 * Screen colors
 */
#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_BROWN         6
#define COLOR_GRAY          7

#define COLOR_DARKGRAY      8
#define COLOR_LIGHTBLUE     9
#define COLOR_LIGHTGREEN    10
#define COLOR_LIGHTCYAN     11
#define COLOR_LIGHTRED      12
#define COLOR_LIGHTMAGENTA  13
#define COLOR_YELLOW        14
#define COLOR_WHITE         15

/* GLOBALS ********************************************************************/
#define ATTR(cFore, cBack)  ((cBack << 4) | cFore)

extern void simplefb_init();
extern void bs_simplefb_putc(unsigned char character);
