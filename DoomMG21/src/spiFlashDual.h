/**
 *  Doom Port on xMG21/Ikea Tradfri
 *  by Nicola Wrachien (next-hack in the comments)
 *
 *  This port is based on the excellent doomhack's GBA Doom Port.
 *  Several data structures and functions have been optimized to fit the
 *  96kB + 12kB memory of xMG21 devices. Z-Depth Light has been restored with almost
 *  no RAM consumption!
 *
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
 *  Dual SPI emulation using two USART and xMG21 PRS.
 *
 */
#ifndef SRC_SPIFLASHDUAL_H_
#define SRC_SPIFLASHDUAL_H_

#include <stdint.h>
#include <stdbool.h>
#include "em_device.h"
#include "main.h"
#include "spi.h"
uint32_t setSpiFlashAddressDualContinuousRead(uint32_t address);
void disableSpiFlashDualContinuousReadMode(void);
uint32_t setSpiFlashDualContinuousReadMode(uint32_t address);
void* spiFlashGetDataDual(void *dest, int len);
void spiFlashProgramForDualRead(uint32_t address, uint8_t *buffer, int size);
void spiFlashDriveStrength();
void spiFlashChipErase();
short spiFlashGetShortDual();
void spiFlashEraseSector(uint32_t address);
int spiFlashGetSize();
extern uint8_t oddByte;
extern bool oddByteAvailable;
extern uint32_t currentFlashAddress;

//
/**
 * Prerequisites: SPI  TX disabled, flushed buffers, both SPIs in input mode.
 *
 * @return
 */
__attribute__((always_inline)) inline uint8_t spiFlashGetByteDual()
{
    // do we need odd byte?
    if (currentFlashAddress & 1)
    {
        if (!oddByteAvailable)
        {
            // wait till RX is full.
            while (!(FIRST_SPI_USART->STATUS & USART_STATUS_RXDATAV));
            // stop feeding with new data. This to avoid get the two SPI out of sync.
            PRS->ASYNC_SWLEVEL = 0;
            //  while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXC));
            // PRS might have some delay, so we need to insert some instructions before actually reading
            currentFlashAddress++;
            oddByteAvailable = false;
            // read data. Now these won't trigger new sends
            uint8_t odd = SECOND_SPI_USART->RXDATA;
            uint8_t even = FIRST_SPI_USART->RXDATA;
            (void) even;
            PRS->ASYNC_SWLEVEL = 1;
            return odd;
        }
        currentFlashAddress++;
        oddByteAvailable = false;
        return oddByte;
    }
    // we are asking for the even byte.
    // wait till RX is full.
    while (!(FIRST_SPI_USART->STATUS & USART_STATUS_RXDATAV));
    // stop feeding with new data. This to avoid get the two SPI out of sync.
    PRS->ASYNC_SWLEVEL = 0;
    //  while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXC));
    // PRS might have some delay, so we need to insert some instructions before actually reading
    currentFlashAddress++;
    oddByteAvailable = true;
    // read data. Now these won't trigger new sends
    oddByte = SECOND_SPI_USART->RXDATA;
    uint8_t even = FIRST_SPI_USART->RXDATA;
    // activate again autosend
    PRS->ASYNC_SWLEVEL = 1;
    return even;
}

#endif /* SRC_SPIFLASHDUAL_H_ */
