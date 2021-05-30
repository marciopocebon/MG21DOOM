/**
 *  Display utility functions for Doom Port on xMG21/Ikea Tradfri lamp.
 *
 *  Copyright (C) 2021 by Nicola Wrachien (next-hack in the comments)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 *  DESCRIPTION:
 *  16-color functions to print general purpose texts.
 *
 */
#include "graphics.h"
#include "display.h"
#include "spi.h"
#include "font8x8.h"
#include "printf.h"
#include "main.h"
//
#define LE2BE16(x) ((uint16_t)(((x >> 8) & 0xFF) | (x << 8)))
#define RGB(r, g, b) (((r >> 3) << (6 + 5)) | ((g >> 2) << 5) | ((b >> 3) << (0)))
//
#define MAX_STRING_SIZE ( SCREEN_WIDTH / FONT_WIDTH + 1)
//
//
const uint16_t palette16[] =    // a gift who those who recognize this palette!
{ LE2BE16(RGB(170, 170, 170)), LE2BE16(RGB(0, 0, 0)), LE2BE16(RGB(255, 255, 255)), LE2BE16(RGB(86, 119, 170)), };
//
//
typedef enum
{
    dm_4bpp = 4, dm_8bpp = 8
} displayMode_t;
//
extern uint8_t displayBuffer[];
//
static uint8_t displayMode = dm_4bpp;
static uint8_t penColor = 1;
static uint8_t penBackground = 0;
static uint8_t line = 0;
//
void setDisplayMode(displayMode_t dm)
{
    displayMode = dm;
}
void setPixel(unsigned int x, unsigned int y, int c)
{
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)
    {
        return;
    }
    switch (displayMode)
    {
        case dm_8bpp:
            // not yet implemented.
            break;
        default:
        case dm_4bpp:
        {
            uint8_t pixel = displayBuffer[x / 2 + SCREEN_WIDTH / 2 * y];
            if (x & 1)
            {
                pixel &= 0x0F;
                pixel |= c << 4;
            }
            else
            {
                pixel &= 0xF0;
                pixel |= c & 0x0F;
            }
            displayBuffer[x / 2 + SCREEN_WIDTH / 2 * y] = pixel;
        }
            break;
    }
}
void displayPutChar(char c, int x, int y)
{
    for (int cy = 0; cy < FONT_HEIGHT; cy++)
    {
        uint8_t fb = font8x8_basic[0x7F & c][cy];
        for (int cx = 0; cx < FONT_WIDTH; cx++)
        {
            setPixel(x + cx, y + cy, (fb & 1) ? penColor : penBackground);
            fb >>= 1;
        }
    }
}
void setDisplayPen(int color, int background)
{
    penColor = color;
    penBackground = background;
}
void displayVPrintf(int x, int y, const char *format, va_list va)
{
    char outString[MAX_STRING_SIZE];
    vsnprintf(outString, MAX_STRING_SIZE, format, va);
    for (int i = 0;
            i < MAX_STRING_SIZE && x < SCREEN_WIDTH && outString[i] <= 0x7F && outString[i] > 0;
            i++)
    {
        displayPutChar(outString[i], x, y);
        x += FONT_WIDTH;
    }
}
void displayPrintf(int x, int y, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    displayVPrintf(x, y, format, va);
    va_end(va);
}
void drawScreen4bpp()
{
    FIRST_SPI_USART->CLKDIV = 128;
    FLASH_NCS_HIGH();
    //
    int n = 0;
    setSingleSPI();
    PRS->ASYNC_SWLEVEL = 0;
    FIRST_SPI_USART->CMD = USART_CMD_TXDIS | USART_CMD_RXDIS | USART_CMD_CLEARRX | USART_CMD_CLEARRX;

    DISPLAY_NCS_LOW();
    FIRST_SPI_USART->CMD = USART_CMD_TXEN;
    while (n < SCREEN_WIDTH * SCREEN_HEIGHT / 2)
    {
        uint8_t biPix = displayBuffer[n++];
        uint16_t data;
        //
        // get first pix
        //
        data = palette16[biPix & 0x0F];
        //
        while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXBL));
        FIRST_SPI_USART->TXDATA = data;
        while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXBL));
        FIRST_SPI_USART->TXDATA = data >> 8;
        //
        // get second pix
        //
        data = palette16[(biPix >> 4) & 0x0F];
        //
        while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXBL));
        FIRST_SPI_USART->TXDATA = data;
        while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXBL));
        FIRST_SPI_USART->TXDATA = data >> 8;

    }
    while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXC));

    DISPLAY_NCS_HIGH();
    // high speed: 40 MHz
    FIRST_SPI_USART->CLKDIV = 0;
}
void displayPrintln(bool update, const char *format, ...)
{
    int y;
    // check if we have space to print.
    if (line < SCREEN_HEIGHT / FONT_HEIGHT)
    {
        y = line * FONT_HEIGHT;
        line++;
    }
    else
    {
        y = SCREEN_HEIGHT - FONT_HEIGHT;
        // printing at the bottom of the screen. move everything up.
        uint32_t *d = (uint32_t*) displayBuffer;
        uint32_t *s = (uint32_t*) (&displayBuffer[SCREEN_WIDTH * FONT_HEIGHT / 2]);
        for (int i = 0; i < SCREEN_WIDTH * (SCREEN_HEIGHT - FONT_HEIGHT) / 8;
                i++)
        {
            *d++ = *s++;
        }
        // clear last row
        uint32_t color = penBackground | (penBackground << 8) | (penBackground << 16) | (penBackground << 24);
        color = color | (color << 4);
        for (int i = 0; i < SCREEN_WIDTH * FONT_HEIGHT / 8; i++)
        {
            *d++ = color;
        }
    }
    // printf will not wrap around
    va_list va;
    va_start(va, format);
    displayVPrintf(0, y, format, va);
    va_end(va);
    if (update)
    {
        drawScreen4bpp();
    }
}
void clearScreen4bpp()
{
    uint32_t *p = (uint32_t*) displayBuffer;
    uint32_t color = penBackground | (penBackground << 8) | (penBackground << 16) | (penBackground << 24);
    color = color | (color << 4);
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT / 8; i++)
    {
        *p++ = color;
    }
    line = 0;
}
