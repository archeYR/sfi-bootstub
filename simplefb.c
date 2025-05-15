/*
 * Debug output via framebuffer. Mostly copypasted ReactOS FreeLoader code.
 */

#include "bootstub.h"
#include "simplefb.h"

/* Character coordinates */
static int x = 0;
static int y = 0;

volatile struct simplefb_data framebufferData = {0};

static void simplefb_init()
{
    /* See SFI page on ReactOS wiki for finding the framebuffer address */
    framebufferData.BaseAddress        = 0x3f000000;
    framebufferData.ScreenWidth        = 768;
    framebufferData.ScreenHeight       = 1024;
    framebufferData.PixelsPerScanLine  = 768;
}

static unsigned long
AttrToSingleColor(unsigned char Attr)
{
    unsigned char Intensity;
    Intensity = (0 == (Attr & 0x08) ? 127 : 255);

    return 0xff000000 |
           (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
           (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
           (0 == (Attr & 0x01) ? 0 : Intensity);
}

static void
AttrToColors(unsigned char Attr, unsigned long *FgColor, unsigned long *BgColor)
{
    *FgColor = AttrToSingleColor(Attr & 0xf);
    *BgColor = AttrToSingleColor((Attr >> 4) & 0xf);
}


static void
ClearScreenColor(unsigned long Color, bool FullScreen)
{
    unsigned long Delta;
    unsigned long Line, Col;
    unsigned long * p;

    Delta = (framebufferData.PixelsPerScanLine * 4 + 3) & ~ 0x3;
    for (Line = 0; Line < framebufferData.ScreenHeight - (FullScreen ? 0 : 2 * TOP_BOTTOM_LINES); Line++)
    {
        p = (unsigned long *) ((char *) framebufferData.BaseAddress + (Line + (FullScreen ? 0 : TOP_BOTTOM_LINES)) * Delta);
        for (Col = 0; Col < framebufferData.ScreenWidth; Col++)
        {
            *p++ = Color;
        }
    }
}

static void
ClearScreen(unsigned char Attr)
{
    unsigned long FgColor, BgColor;

    AttrToColors(Attr, &FgColor, &BgColor);
    ClearScreenColor(BgColor, false);
}

/* This function expects 8x16 binary font */
static void
OutputChar(unsigned char Char, unsigned X, unsigned Y, unsigned long FgColor, unsigned long BgColor)
{
    unsigned char *FontPtr;
    unsigned long *Pixel;
    unsigned char Mask;
    unsigned Line;
    unsigned Col;
    unsigned long Delta;
    Delta = (framebufferData.PixelsPerScanLine * 4 + 3) & ~ 0x3;
    FontPtr = (u8 *)FONT_OFFSET + Char * 16;
    Pixel = (unsigned long *) ((char *) framebufferData.BaseAddress +
            (Y * CHAR_HEIGHT + TOP_BOTTOM_LINES) *  Delta + X * CHAR_WIDTH * 4);

    for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
        Mask = 0x80;
        for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
            Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? FgColor : BgColor);
            Mask = Mask >> 1;
        }
        Pixel = (unsigned long *) ((char *) Pixel + Delta);
    }
}

static void
ScrollUp()
{
    unsigned long BgColor, Dummy;
    unsigned long Delta;
    Delta = (framebufferData.PixelsPerScanLine * 4 + 3) & ~ 0x3;
    unsigned long PixelCount = framebufferData.ScreenWidth * CHAR_HEIGHT *
                       (((framebufferData.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT) - 1);
    unsigned long *Src = (unsigned long *)((unsigned char *)framebufferData.BaseAddress + (CHAR_HEIGHT + TOP_BOTTOM_LINES) * Delta);
    unsigned long *Dst = (unsigned long *)((unsigned char *)framebufferData.BaseAddress + TOP_BOTTOM_LINES * Delta);

    AttrToColors(ATTR(COLOR_WHITE, COLOR_BLACK), &Dummy, &BgColor);

    while (PixelCount--)
        *Dst++ = *Src++;

    for (PixelCount = 0; PixelCount < framebufferData.ScreenWidth * CHAR_HEIGHT; PixelCount++)
        *Dst++ = BgColor;
}

static void
PutChar(int Ch, unsigned char Attr, unsigned X, unsigned Y)
{
    unsigned long FgColor = 0;
    unsigned long BgColor = 0;
    if (Ch != 0)
    {
        AttrToColors(Attr, &FgColor, &BgColor);
        OutputChar(Ch, X, Y, FgColor, BgColor);
    }
}

void
bs_simplefb_putc(unsigned char character)
{
	if (framebufferData.BaseAddress == 0)
	{
		simplefb_init();
		ClearScreen(ATTR(COLOR_WHITE, COLOR_BLACK));
	}

    if (y >= framebufferData.ScreenHeight/CHAR_HEIGHT)
    {
        ScrollUp();
        y--;
    }

    if (character == '\n' || x >= framebufferData.ScreenWidth/CHAR_WIDTH)
    {
        y++;
        x = 0;
        if (character == '\n')
            return;
    }

    PutChar(character, ATTR(COLOR_GRAY, COLOR_BLACK), x++, y);
}
