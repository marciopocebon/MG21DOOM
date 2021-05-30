/*  Doom Port on xMG21/Ikea Tradfri
 *  by Nicola Wrachien (next-hack in the comments)
 *
 *  This port is based on the excellent doomhack's GBA Doom Port.
 *  Several data structures and functions have been optimized to fit the
 *  96kB + 12kB memory of xMG21 devices. Z-Depth Light has been restored with almost
 *  no RAM consumption!
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *  Copyright (C) 2021 Nicola Wrachien (next-hack in the comments)
 *  on xMG21/Ikea Tradfri port.
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
 * DESCRIPTION:
 *  DOOM graphics stuff.
 *  next-hack: and also key handling, don't ask me why they were put in
 *  i_video.c. Key handling and graphics I/O modified according HW...
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <math.h>
#include "main.h"

#include "doomstat.h"
#include "doomdef.h"
#include "doomtype.h"
#include "v_video.h"
#include "r_draw.h"
#include "d_main.h"
#include "d_event.h"
#include "i_video.h"
#include "i_sound.h"
#include "z_zone.h"
#include "s_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "st_stuff.h"
#include "lprintf.h"
#include "doomdef.h"

#include "global_data.h"
#include "spi.h"
#include "display.h"
#include "em_cmu.h"

#define NO_PALETTE_CHANGE 255

uint8_t displayBuffer[96 * SCREENWIDTH_PHYSICAL] =
{ 0 };
extern short openings[];

uint8_t keysDown()
{
    uint8_t keys = 0;
    // Readout data from 74HC165
    // Disable FLASH
    FLASH_NCS_HIGH();
    //goto after;
    // Disable Display
    DISPLAY_NCS_HIGH();
    // Set display pin to high, which means, shift enabled. (Keys have been already sampled)
    // low speed: 5MHz
    FIRST_SPI_USART->CLKDIV = 7 << 8;
    setSingleSPI();
    PRS->ASYNC_SWLEVEL = 0;
    FIRST_SPI_USART->CMD = USART_CMD_TXDIS | USART_CMD_RXDIS | USART_CMD_CLEARRX | USART_CMD_CLEARRX;
    DISPLAY_DC_LOW();
    GPIO->USARTROUTE[FIRST_SPI_NUMBER].RXROUTE = ((uint32_t) SR_OUT_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) | (SR_OUT_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
    // Give a low pulse to load data
    DISPLAY_DC_HIGH();
    keys = spiread(0);
    // Restore everything
    // restore speed
    // high speed: 40 MHz
    FIRST_SPI_USART->CLKDIV = 0;
    // normal mode: double spi in.
    setDoubleSpiIn();
    FLASH_NCS_LOW();
#if WSTKBOARD
    return keys;
#else
  return ~keys;
#endif
}
//
// I_StartTic
//
void I_StartTic(void)
{
    static uint16_t oldGameKeyState = 0;
    // get which keys are currently pressed
    uint8_t hwKeyState = keysDown();
    // translate between the hardware key state and the game key state
    uint16_t gameKeyState = 0;

    if (hwKeyState & KEY_ALT)
    {
        if (hwKeyState & KEY_RIGHT)
            gameKeyState |= 1 << KEYD_SR;
        if (hwKeyState & KEY_LEFT)
            gameKeyState |= 1 << KEYD_SL;
        if (hwKeyState & KEY_USE)
            gameKeyState |= 1 << KEYD_MENU;
        if (hwKeyState & KEY_CHGW)
            gameKeyState |= 1 << KEYD_CHGWDOWN;
        if (hwKeyState & KEY_FIRE)
            gameKeyState |= 1 << KEYD_MAP1;
    }
    else
    {
        if (hwKeyState & KEY_RIGHT)
            gameKeyState |= 1 << KEYD_RIGHT;
        if (hwKeyState & KEY_LEFT)
            gameKeyState |= 1 << KEYD_LEFT;
        if (hwKeyState & KEY_USE)
            gameKeyState |= 1 << KEYD_USE;
        if (hwKeyState & KEY_CHGW)
            gameKeyState |= 1 << KEYD_CHGW;
        if (hwKeyState & KEY_FIRE)
            gameKeyState |= 1 << KEYD_FIRE;
    }

    if (hwKeyState & KEY_UP)
        gameKeyState |= 1 << KEYD_UP;
    if (hwKeyState & KEY_DOWN)
        gameKeyState |= 1 << KEYD_DOWN;
    // Get which keys have changed since last time
    uint16_t keys_changed = oldGameKeyState ^ gameKeyState;

    event_t ev;
    ev.type = ev_keydown;
    for (int i = 0; i < NUMKEYS; i++)
    {
        if ((1 & (keys_changed >> i)) && (1 & (gameKeyState >> i)))
        {
            ev.data1 = i;
            D_PostEvent(&ev);
        }
    }
    ev.type = ev_keyup;
    for (int i = 0; i < NUMKEYS; i++)
    {
        if ((1 & (keys_changed >> i)) && !(1 & (gameKeyState >> i)))
        {
            ev.data1 = i;
            D_PostEvent(&ev);
        }
    }
    oldGameKeyState = gameKeyState;
}

boolean I_StartDisplay(void)
{
    _g->screens[0].data = displayBuffer;
    // Same with base row offset.
    drawvars.byte_topleft = displayBuffer;
    return true;
}

/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
// Palette stuff.
//

static void I_UploadNewPalette(int pal)
{
    // We need the palette only when we are actually drawing.
    // but when we are drawing we do not need the openings data anymore.
    // so we reuse that zone as buffer (that zone is larger than 512 bytes).
    _g->current_palette = (unsigned short*) &openings[0];
    //
    if (_g->gamma)
    {
        for (int i = 0; i < 256; i++)
        {
            uint16_t r = gammatable[_g->gamma][p_wad_immutable_flash_data->palette_lump[pal * 256 * 3 + 3 * i]] >> 3;
            uint16_t g = gammatable[_g->gamma][p_wad_immutable_flash_data->palette_lump[pal * 256 * 3 + 3 * i + 1]] >> 2;
            uint16_t b = gammatable[_g->gamma][p_wad_immutable_flash_data->palette_lump[pal * 256 * 3 + 3 * i + 2]] >> 3;
            uint16_t rgb = (r << (6 + 5)) | (g << 5) | (b << (0));
            rgb = (rgb >> 8) | (rgb << 8);
            _g->current_palette[i] = rgb;
        }
    }
    else
    {   // save some cycles for those playing with 0 gamma.
        for (int i = 0; i < 256; i++)
        {
            uint16_t r = p_wad_immutable_flash_data->palette_lump[pal * 256 * 3 + 3 * i] >> 3;
            uint16_t g = p_wad_immutable_flash_data->palette_lump[pal * 256 * 3 + 3 * i + 1] >> 2;
            uint16_t b = p_wad_immutable_flash_data->palette_lump[pal * 256 * 3 + 3 * i + 2] >> 3;
            uint16_t rgb = (r << (6 + 5)) | (g << 5) | (b << (0));
            rgb = (rgb >> 8) | (rgb << 8);
            _g->current_palette[i] = rgb;
        }

    }

}

//////////////////////////////////////////////////////////////////////////////
// Graphics API
//

void I_FinishUpdateBlock(void)
{
    I_UploadNewPalette(_g->newpal);

    int n = 0;
    int count = (draw_stopy - draw_starty + 1) * SCREENWIDTH_PHYSICAL;
    //
    // low speed: 30 MHz
    FIRST_SPI_USART->CLKDIV = 128;
    FLASH_NCS_HIGH();

    setSingleSPI();
    PRS->ASYNC_SWLEVEL = 0;
    FIRST_SPI_USART->CMD = USART_CMD_TXDIS | USART_CMD_RXDIS | USART_CMD_CLEARRX | USART_CMD_CLEARRX;

    DISPLAY_NCS_LOW();
    FIRST_SPI_USART->CMD = USART_CMD_TXEN;

    while (n < count)
    {
        uint16_t data = _g->current_palette[displayBuffer[n++]];
        while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXBL));
        FIRST_SPI_USART->TXDATA = data;
        while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXBL));
        FIRST_SPI_USART->TXDATA = data >> 8;
    }
    while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXC));

    DISPLAY_NCS_HIGH();
    // high speed: 40 MHz
    FIRST_SPI_USART->CLKDIV = 0;
    setDoubleSpiIn();
    FLASH_NCS_LOW();

}

//
// I_SetPalette
//
void I_SetPalette(int pal)
{
    _g->newpal = pal;
}

