/* Doom Port on xMG21/Ikea Tradfri
 *  by Nicola Wrachien (next-hack in the comments)
 *
 *  This port is based on the excellent doomhack's GBA Doom Port.
 *  Several data structures and functions have been optimized to fit the
 *  96kB + 12kB memory of xMG21 devices. Z-Depth Light has been restored with almost
 *  no RAM consumption!
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
#include <string.h>
#include "i_spi_support.h"
#include "spiFlashDual.h"
uint32_t spiFlashSetAddress(uint32_t address)
{
    return setSpiFlashAddressDualContinuousRead(address);
}
void* spiFlashGetData(void *dest, unsigned int length)
{
    return spiFlashGetDataDual(dest, length);
}

