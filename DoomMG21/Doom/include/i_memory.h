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
 * next-hack: packed (24) and 16 bit pointers utility functions and other stuff
 */

#ifndef DOOM_INCLUDE_I_MEMORY_H_
#define DOOM_INCLUDE_I_MEMORY_H_
#include <stdint.h>

#define RAM_PTR_BASE 0x20000000UL
#define SEQRAM_PTR_BASE 0xA0000000UL
#define EXT_FLASH_BASE 0x90000000
#define FLASH_PTR_BASE 0
#define PACKED_MEMORY_ADDRESS0 RAM_PTR_BASE
#define PACKED_MEMORY_ADDRESS1 SEQRAM_PTR_BASE
#define PACKED_MEMORY_ADDRESS2 EXT_FLASH_BASE
#define PACKED_MEMORY_ADDRESS3 FLASH_PTR_BASE
//
#define WAD_ADDRESS (EXT_FLASH_BASE + 4)
#define FLASH_CODE_KB 344
#define FLASH_ADDRESS (0x00000000 + FLASH_CODE_KB * 1024)
#define FLASH_IMMUTABLE_REGION_ADDRESS FLASH_ADDRESS
#define FLASH_IMMUTABLE_REGION 0
#define FLASH_LEVEL_REGION 1
#define FLASH_CACHE_REGION_SIZE ((1024 - FLASH_CODE_KB) * 1024)
#define FLASH_BLOCK_SIZE 8192
//
#define DEBUG_GETSHORTPTR 0
//
#define isOnExternalFlash(a) (((uint32_t) a & EXT_FLASH_BASE) == EXT_FLASH_BASE)

typedef struct
{
    uint8_t addrBytes[3];
} __attribute((packed)) packedAddress_t;

static inline void* getLongPtr(unsigned short shortPointer)
{ // Special case: short pointer being all 0 => NULL
    if (!shortPointer)
        return 0;
    if (shortPointer & 0x8000)
        return (void*) (((unsigned int) (shortPointer & 0x7FFF) << 2) | SEQRAM_PTR_BASE);
    else
        return (void*) (((unsigned int) shortPointer << 2) | RAM_PTR_BASE);
}
static inline unsigned short getShortPtr(void *longPtr)
{

    if ((unsigned int) longPtr & 0x80000000)
    {
#if DEBUG_GETSHORTPTR
      if ( ((unsigned int)longPtr) >= (SEQRAM_PTR_BASE + 12*1024) || (((unsigned int)longPtr) & 3) ||  ((unsigned int)longPtr) < SEQRAM_PTR_BASE )
      {
        printf("Error, longPtr 0x%08X line %d File %s\r\n", (unsigned int) longPtr, __LINE__, __FILE__);
        while(1);
      }
    #endif
        return (((unsigned int) longPtr) >> 2) | 0x8000;
    }
    else
    {
#if DEBUG_GETSHORTPTR
      if ( ((unsigned int)longPtr) >= (RAM_PTR_BASE + 96*1024) || (((unsigned int)longPtr) & 3))
      {
        printf("Error, longPtr 0x%08X line %d File %s\r\n", (unsigned int) longPtr, __LINE__, __FILE__);
        while(1);
      }
    #endif
        return ((unsigned int) longPtr) >> 2;
    }
}

static inline packedAddress_t getPackedAddress(void *addr)
{
    packedAddress_t ret;
    unsigned int intaddr = (uint32_t) addr;
    uint8_t highaddr = intaddr >> 24;
    uint8_t highAddrBits = 0;
    ret.addrBytes[0] = intaddr;
    ret.addrBytes[1] = intaddr >> 8;
    switch (highaddr)
    {
        case (PACKED_MEMORY_ADDRESS0 >> 24) & 0xFF:
            highAddrBits = 0x0;
            break;
        case (PACKED_MEMORY_ADDRESS1 >> 24) & 0xFF:
            highAddrBits = 0x40;
            break;
        case (PACKED_MEMORY_ADDRESS2 >> 24) & 0xFF:
            highAddrBits = 0x80;
            break;
        case (PACKED_MEMORY_ADDRESS3 >> 24) & 0xFF:
            highAddrBits = 0xC0;
            break;
    }
    ret.addrBytes[2] = ((intaddr >> 16) & 0x3F) | highAddrBits;
    return ret;
}

static inline void* unpackAddress(packedAddress_t a) // passed by copy, because it is shorted than a pointer!
{
    const uint32_t addresses[] =
    { PACKED_MEMORY_ADDRESS0, PACKED_MEMORY_ADDRESS1, PACKED_MEMORY_ADDRESS2, PACKED_MEMORY_ADDRESS3 };

    if (!a.addrBytes[2] && !a.addrBytes[1] && !a.addrBytes[0])
    {
        return NULL;
    }
    return (void*) (addresses[a.addrBytes[2] >> 6] | (a.addrBytes[0]) | (a.addrBytes[1] << 8) | ((a.addrBytes[2] & 0x3F) << 16));
}

#endif /* DOOM_INCLUDE_I_MEMORY_H_ */
