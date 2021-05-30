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
 *  DESCRIPTION:
 *  next-hack: Some wrapper functions, to use SPI routines.
 *  These were implemented for a smooth transition between memory-mapped and
 *  SPI-based accesses on STM32 devices, and later xMG21 port.
 *  Memory mapped code has been removed.
 */
#ifndef I_SPI_SUPPORT_H_
#define I_SPI_SUPPORT_H_
#include <stdint.h>
#include "spi.h"
#include "spiFlashDual.h"
//
static inline uint8_t spiFlashGetByte()
{
    return spiFlashGetByteDual();
}
static inline short spiFlashGetShort()
{
    return spiFlashGetShortDual();
}
uint32_t spiFlashSetAddress(uint32_t address);
static inline uint8_t spiFlashGetByteFromAddress(const void *addr)
{
    setSpiFlashAddressDualContinuousRead((uint32_t) addr);
    return spiFlashGetByteDual();
}
void* spiFlashGetData(void *dest, unsigned int length);
#endif /* SOURCE_I_SPI_SUPPORT_H_ */
