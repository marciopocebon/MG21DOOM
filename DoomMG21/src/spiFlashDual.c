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
#include "spiFlashDual.h"
//
#define SPI_FLASH_WRITE_ENABLE_CMD 0x06
#define SPI_FLASH_PAGE_PROGRAM_CMD 0x02
#define SPI_FLASH_STATUS_REGISTER_READ_CMD 0x05
#define SPI_FLASH_CHIP_ERASE 0xC7
#define SPI_FLASH_SECTOR_ERASE 0x20
#define SPI_FLASH_MFG_ID 0x90
#define SPI_FLASH_STATUS_REGISTER_BUSY 1
#define SPI_FLASH_PAGE_SIZE 256
// ID for common flash sizes
#define ID_4M 0x15
#define ID_8M 0x16
#define ID_16M 0x17
//
static void spiFlashWaitBusy();
static void spiFlashWriteEnable();
static inline uint32_t interleave32(uint16_t even, uint16_t odd);
int32_t flashSize = -1;
uint8_t oddByte;
bool oddByteAvailable;
uint32_t currentFlashAddress = 0xFFFFFFFF;

int spiFlashGetSize()
{
    if (-1 == flashSize)
    {
        flashSize = 0;
        setSingleSPI();
        FLASH_NCS_LOW();
        spiread(SPI_FLASH_MFG_ID);
        spiread(0);
        spiread(0);
        spiread(0);
        spiread(0);
        uint8_t id;
        id = spiread(0);
        flashSize = 4096 * 1024 << (id - ID_4M);
        FLASH_NCS_HIGH();
    }
    return flashSize;
}

void spiFlashChipErase()
{
    setSingleSPI();
    FLASH_NCS_HIGH();
    spiFlashWriteEnable();
    FLASH_NCS_LOW();
    spiread(SPI_FLASH_CHIP_ERASE);
    FLASH_NCS_HIGH();
    spiFlashWaitBusy();
}
void spiFlashEraseSector(uint32_t address)
{
    setSingleSPI();
    FLASH_NCS_HIGH();
    spiFlashWriteEnable();
    FLASH_NCS_LOW();
    // command and address
    spiread(SPI_FLASH_SECTOR_ERASE);
    spiread(address >> 16);
    spiread(address >> 8);
    spiread(address);
    //
    FLASH_NCS_HIGH();
    spiFlashWaitBusy();
}
void spiFlashDriveStrength()
{
    setSingleSPI();
    // set enable write of volatile SR
    FLASH_NCS_LOW();
    spiread(0x50);
    FLASH_NCS_HIGH();
    FLASH_NCS_LOW();
    spiread(0x11);
    spiread(0x0);
    FLASH_NCS_HIGH();
    spiFlashWaitBusy();
}

static void spiFlashWaitBusy()
{
    uint8_t result;
    FLASH_NCS_LOW();
    result = spiread(SPI_FLASH_STATUS_REGISTER_READ_CMD);
    do
    {
        result = spiread(0xFF);

    } while (result & SPI_FLASH_STATUS_REGISTER_BUSY);
    FLASH_NCS_HIGH();

}
static void spiFlashWriteEnable()
{
    FLASH_NCS_LOW();
    spiread(SPI_FLASH_WRITE_ENABLE_CMD);
    FLASH_NCS_HIGH();

}
static inline uint8_t swapNibbles(uint8_t x)
{
    return (x >> 4) | (x << 4);
}
/**
 * @brief reads a short from external flash
 * Prerequisites: both SPI are in read mode, TX disabled.
 * @return the short to be read
 */
short spiFlashGetShortDual()
{
    uint16_t result;
    // do we need odd byte too?
    if (currentFlashAddress & 1)
    {
        if (!oddByteAvailable)
        {
            // two words to read, discard the first byte.
            // wait till RX is full. We will have all the words we need.
            while (!(FIRST_SPI_USART->STATUS & USART_STATUS_RXFULL));
            // stop feeding with new data. This to avoid get the two SPI out of sync.
            PRS->ASYNC_SWLEVEL = 0;
            //  while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXC));
            // PRS might have some delay, so we need to insert some instructions before actually reading
            currentFlashAddress += 2;
            oddByteAvailable = true;
            // read data. Now these won't trigger new sends
            uint8_t odd = SECOND_SPI_USART->RXDATA;
            uint8_t even = FIRST_SPI_USART->RXDATA;
            oddByte = SECOND_SPI_USART->RXDATA;
            even = FIRST_SPI_USART->RXDATA;
            PRS->ASYNC_SWLEVEL = 1;
            result = odd | (even << 8);
            // activate again autosend
            return (short) result;
        }
        else
        {  // only a single dual byte read.
            while (!(FIRST_SPI_USART->STATUS & USART_STATUS_RXFULL));
            // stop feeding with new data. This to avoid get the two SPI out of sync.
            PRS->ASYNC_SWLEVEL = 0;
            //  while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXC));
            // PRS might have some delay, so we need to insert some instructions before actually reading
            currentFlashAddress += 2;
            oddByteAvailable = true;
            // read data. Now these won't trigger new sends
            uint8_t odd = oddByte;
            uint8_t even = FIRST_SPI_USART->RXDATA;
            oddByte = SECOND_SPI_USART->RXDATA;
            PRS->ASYNC_SWLEVEL = 1;
            result = odd | (even << 8);
            // activate again autosend
            return (short) result;
        }
    }
    // wait till RX is full.
    while (!(FIRST_SPI_USART->STATUS & USART_STATUS_RXDATAV));
    // stop feeding with new data. This to avoid get the two SPI out of sync.
    PRS->ASYNC_SWLEVEL = 0;
    //  while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXC));
    // PRS might have some delay, so we need to insert some instructions before actually reading
    currentFlashAddress += 2;
    oddByteAvailable = false;
    // read data. Now these won't trigger new sends
    uint8_t odd = SECOND_SPI_USART->RXDATA;
    uint8_t even = FIRST_SPI_USART->RXDATA;
    PRS->ASYNC_SWLEVEL = 1;
    result = even | (odd << 8);
    // activate again autosend
    return (short) result;
}
/**
 * @brief program the spi flash, by interleaving bits, so that reading with
 * two SPIs will yield correct bit value
 * @param address
 * @param buffer
 * @param size
 */
void spiFlashProgramForDualRead(uint32_t address, uint8_t *buffer, int size)
{
    if (!size)
        return;
    setSingleSPI();
    int written = 0;
    int i = 0;
    while (written < size)
    {
        FLASH_NCS_HIGH();
        spiFlashWriteEnable();
        FLASH_NCS_LOW();
        spiread(SPI_FLASH_PAGE_PROGRAM_CMD);
        spiread(address >> 16);
        spiread(address >> 8);
        spiread(address);
        // we can only program within one page at once.
        int maxWrite = SPI_FLASH_PAGE_SIZE - (address & (SPI_FLASH_PAGE_SIZE - 1));
        for (i = 0; i + written < size && i < maxWrite; i += 2)
        {
            uint16_t interleaved = interleave32(swapNibbles(buffer[i + written]), swapNibbles(buffer[i + 1 + written]));
            spiread(interleaved);
            spiread(interleaved >> 8);
        }
        FLASH_NCS_HIGH();
        spiFlashWaitBusy();
        written += i;
        address += i;
    }
}
void* spiFlashGetDataDual(void *dest, int len)
{
    uint8_t *buffer = dest;
    if (!len)
        return buffer;
    // stop feeding with new data. This to avoid get the two SPI out of sync.
    PRS->ASYNC_SWLEVEL = 0;

    bool skipFirstByte;
    // if odd byte is not available, but we have to read it, then we need to read
    // the whole halfword and skip the even byte.
    skipFirstByte = (currentFlashAddress & 1) && !oddByteAvailable;
    // get the final address
    currentFlashAddress += len;
    //
    if (oddByteAvailable) // oddByteAvailable is 1 only if currentFlashAddress is odd.
    {
        *buffer++ = oddByte;
        len--;
    }
    if (len + skipFirstByte <= 6)
    {
        while (len > 0)
        {
            // read two bytes
            uint8_t evenByte;
            evenByte = FIRST_SPI_USART->RXDATA;
            oddByte = SECOND_SPI_USART->RXDATA;
            // skip the first byte
            if (!skipFirstByte)
            {
                *buffer++ = evenByte;
                len--;
            }
            else
            {
                skipFirstByte = false;
            }
            if (len > 0)
            {
                *buffer++ = oddByte;
                len--;
            }
        }
        oddByteAvailable = currentFlashAddress & 1;
        PRS->ASYNC_SWLEVEL = 1;
    }
    else
    {
        len -= 8 - skipFirstByte;
        uint8_t even, odd;
        even = FIRST_SPI_USART->RXDATA;
        odd = SECOND_SPI_USART->RXDATA;
        if (!skipFirstByte)
        {
            *buffer++ = even;
        }
        else
        {
            skipFirstByte = false;
        }
        *buffer++ = odd;
        *buffer++ = FIRST_SPI_USART->RXDATA;
        *buffer++ = SECOND_SPI_USART->RXDATA;
        *buffer++ = FIRST_SPI_USART->RXDATA;
        *buffer++ = SECOND_SPI_USART->RXDATA;
        // now we can start read
        __disable_irq();
        NVIC->ICPR[(((uint32_t) USART_TEST_IRQn) >> 5UL)] = (uint32_t) (1UL << (((uint32_t) USART_TEST_IRQn) & 0x1FUL));
        NVIC_EnableIRQ(USART_TEST_IRQn);
        // read more bytes...
        // this loop is critical so we do it in ASM.
        if (len > 0)
        {
            // note: self initialization to remove uninitialized warnings.
            // We actually do not care about tmp0 and tmp1, we only need it as a placeholder
            // on the asm side.
            uint8_t tmp0 = tmp0, tmp1 = tmp1;
            // todo: use CBZ.
            __asm volatile
            (
                    ".align (4)\n\t"
                    "MOV %[odd], #1\n\t"
                    "STR %[odd], [%[PRS_ASYNC_SWLEVEL]]\n\t"
                    //"SUB %[buffer], #1\n\t"
                    "sendloop%=:\n\t"
                    "WFI\n\t"// Wait for interrupt
                    "LDR %[even], [%[FIRSTSPI], #0x24]\n\t"// load data from the second SPI. Why loading a word, if we then use a byte? Because the offset is 0x24, which would be too high for a 16bit thumb LDRB instruction.
                    "LDR %[odd], [%[SECONDSPI], #0x24]\n\t"// load data from the second SPI. Why loading a word, if we then use a byte? Because the offset is 0x24, which would be too high for a 16bit thumb LDRB instruction.
                    "STR %[icprVal], [%[NVICICPR]]\n\t"// clear ISR
                    "ORR  %[even], %[even], %[odd], LSL #8\n\t"// ORR
                    "STRH %[even],[%[buffer]],#2\n\t"// store to the pointer and post increment by 2
                    "SUBS %[len],#2\n\t"// decrement by 2
                    "BGT sendloop%=\n\t"// if 0 go to loop
                    :[buffer] "+l" (buffer),
                    [even] "+l" (tmp0),
                    [odd] "+l" (tmp1),
                    [len] "+l" (len)
                    : [icprVal] "l" ((uint32_t)(1UL << (((uint32_t)USART_TEST_IRQn) & 0x1FUL))),
                    [FIRSTSPI] "l" (FIRST_SPI_USART),
                    [SECONDSPI] "l" (SECOND_SPI_USART),
                    [NVICICPR] "l" (&NVIC->ICPR[(((uint32_t)USART_TEST_IRQn) >> 5UL)]),
                    [PRS_ASYNC_SWLEVEL] "r" (&PRS->ASYNC_SWLEVEL)
                    :
            );
        }
        else
        {
            PRS->ASYNC_SWLEVEL = 1;
        }
        //
        // get last two bytes
        __WFI();
        even = FIRST_SPI_USART->RXDATA;
        odd = SECOND_SPI_USART->RXDATA;
        //
        *buffer++ = even;
        if ((len == 0))
        {
            *buffer++ = odd;
            oddByteAvailable = false;
        }
        else
        {
            oddByte = odd;
            oddByteAvailable = true;
        }
        //
        NVIC_DisableIRQ(USART_TEST_IRQn);
        __enable_irq();
    }
    return dest;
}
static inline uint32_t interleave32(uint16_t even, uint16_t odd)
{

    even = (even | (even << 8)) & 0x00FF00FF;
    even = (even | (even << 4)) & 0x0F0F0F0F;
    even = (even | (even << 2)) & 0x33333333;
    even = (even | (even << 1)) & 0x55555555;

    odd = (odd | (odd << 8)) & 0x00FF00FF;
    odd = (odd | (odd << 4)) & 0x0F0F0F0F;
    odd = (odd | (odd << 2)) & 0x33333333;
    odd = (odd | (odd << 1)) & 0x55555555;

    return even | (odd << 1);
}
static inline void deinterleave32(uint32_t input, uint16_t *even, uint16_t *odd)
{
    uint32_t x;
    // deinterleave even bits
    x = input & 0x55555555;
    x = (x | (x >> 1)) & 0x33333333;
    x = (x | (x >> 2)) & 0x0F0F0F0F;
    x = (x | (x >> 4)) & 0x00FF00FF;
    x = (x | (x >> 8)) & 0x0000FFFF;
    *even = (uint16_t) x;
    // deinterleave odd bits
    x = (input >> 1) & 0x55555555;
    x = (x | (x >> 1)) & 0x33333333;
    x = (x | (x >> 2)) & 0x0F0F0F0F;
    x = (x | (x >> 4)) & 0x00FF00FF;
    x = (x | (x >> 8)) & 0x0000FFFF;
    *odd = (uint16_t) x;
}
//
uint32_t setSpiFlashDualContinuousReadMode(uint32_t address)
{
    // put async PRS level to 0, to disable any continuous transmission
    // and to allow pulse triggers to work correctly
    PRS->ASYNC_SWLEVEL = 0;
    // Pulse on CS
    FLASH_NCS_HIGH();
    FLASH_NCS_LOW();
    // Terminate any autotx
    FIRST_SPI_USART->TRIGCTRL_CLR = USART_TRIGCTRL_AUTOTXTEN;
    SECOND_SPI_USART->TRIGCTRL_CLR = USART_TRIGCTRL_AUTOTXTEN;
    // clear buffers
    FIRST_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_RXDIS;
    SECOND_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_RXDIS;
    //
    setSingleSPI();
    // send fast read dual io mode
    while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXBL));
    FIRST_SPI_USART->TXDATA = 0xBB;
    // start transmission
    //
    PRS->ASYNC_SWPULSE = 1;   // pulse on channel 0 to send data concurrently.
    while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXC));
    // stop transmission
    FIRST_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_RXDIS;
    SECOND_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_RXDIS;
    //
    currentFlashAddress = address;
    // set DSPI in dual out mode
    setDouleSpiOut();
    // get the even address and deinterleave it in two bytes.
    uint32_t physicalAddress = address & ~1; // only even address can be set, otherwise we cannot read correcly bytes.
    physicalAddress = (physicalAddress << 8) | 0x20; // 0x20 is required for continuous address mode without having to send address the read command
    uint16_t ae, ao;
    deinterleave32(physicalAddress, &ae, &ao);
    // Note, the SPI TX are disabled, so we can comfortably fill the data registers!
    // reverse byte order (little to big endian). Unfortunately ae = (ae >> 8) | (ae << 8); is not converted to a single instruction
    // TODO: this can be done in hw too!
    __asm volatile ("REV16 %0, %0\n\t" : "+r" (ae));
    FIRST_SPI_USART->TXDOUBLE = ae;
    __asm volatile("REV16 %0, %0\n\t" : "+r" (ao));
    SECOND_SPI_USART->TXDOUBLE = ao;
    //
    PRS->ASYNC_SWPULSE = 1;   // pulse on channel 0 to send data concurrently.
    //
    oddByteAvailable = false;
    // wait till add data have been transmitted
    while (!(SECOND_SPI_USART->STATUS & USART_STATUS_TXC));
    // set both SPIs as input,
    // disable TX and clear RX on both
    FIRST_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_CLEARTX | USART_CMD_RXDIS;
    SECOND_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_CLEARTX | USART_CMD_RXDIS;
    //
    setDoubleSpiIn();
    // Now activate autotx
    FIRST_SPI_USART->TRIGCTRL_SET = USART_TRIGCTRL_AUTOTXTEN;
    SECOND_SPI_USART->TRIGCTRL_SET = USART_TRIGCTRL_AUTOTXTEN;
    // and start grabbing first data.
    PRS->ASYNC_SWLEVEL = 1;
    //
    return address;
}
void disableSpiFlashDualContinuousReadMode(void)
{
    // put async PRS level to 0, to disable any continuous transmission
    // and to allow pulse triggers to work correctly
    PRS->ASYNC_SWLEVEL = 0;
    // Pulse on CS
    GPIO->P_SET[SPI_PORT].DOUT = 1 << FLASH_NCS_PIN;
    GPIO->P_CLR[SPI_PORT].DOUT = 1 << FLASH_NCS_PIN;
    // To disable SPI, continuous read mode, we need to send a dummy address read
    // with M5-M4 != 0b10. We can set therefore SPI in read mode and write 4 bytes.
    // the input will be pull up.
    setDoubleSpiIn();
    FIRST_SPI_USART->TXDOUBLE = 0;
    SECOND_SPI_USART->TXDOUBLE = 0;
    // start transmission
    PRS->ASYNC_SWPULSE = 1;   // pulse on channel 0 to send data concurrently.
    while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXC) || !(SECOND_SPI_USART->STATUS & USART_STATUS_TXC));
    FIRST_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_RXDIS;
    SECOND_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_RXDIS;
    // disable flash.
    GPIO->P_SET[SPI_PORT].DOUT = 1 << FLASH_NCS_PIN;
}
//
uint32_t setSpiFlashAddressDualContinuousRead(uint32_t address)
{
    // put async PRS level to 0, to disable any continuous transmission
    // and to allow pulse triggers to work correctly
    PRS->ASYNC_SWLEVEL = 0;
    // Pulse on CS
    GPIO->P_SET[SPI_PORT].DOUT = 1 << FLASH_NCS_PIN;
    GPIO->P_CLR[SPI_PORT].DOUT = 1 << FLASH_NCS_PIN;
    //
    FIRST_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_RXDIS;
    SECOND_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_RXDIS;
    //
    FIRST_SPI_USART->TRIGCTRL_CLR = USART_TRIGCTRL_AUTOTXTEN;
    SECOND_SPI_USART->TRIGCTRL_CLR = USART_TRIGCTRL_AUTOTXTEN;
    //
    currentFlashAddress = address;
    // set DSPI in dual out mode
    setDouleSpiOut();
    // get the even address and deinterleave it in two bytes.
    uint32_t physicalAddress = address & ~1; // only even address can be set, otherwise we cannot read correcly bytes.
    physicalAddress = (physicalAddress << 8) | 0x20; // 0x20 is required for continuous address mode without having to send address the read command
    uint16_t ae, ao;
    deinterleave32(physicalAddress, &ae, &ao);
    // Note, the SPI TX are disabled, so we can comfortably fill the data registers!
    // reverse byte order (little to big endian). Unfortunately ae = (ae >> 8) | (ae << 8); is not converted to a single instruction
    __asm volatile ("REV16 %0, %0\n\t" : "+r" (ae));
    FIRST_SPI_USART->TXDOUBLE = ae;
    __asm volatile("REV16 %0, %0\n\t" : "+r" (ao));
    SECOND_SPI_USART->TXDOUBLE = ao;
    //
    PRS->ASYNC_SWPULSE = 1;   // pulse on channel 0 to send data concurrently.
    oddByteAvailable = false;
    // wait till add data have been transmitted
    while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXC) || !(SECOND_SPI_USART->STATUS & USART_STATUS_TXC));
    // set both SPIs as input,
    setDoubleSpiIn();
    // disable TX and clear RX on both
    FIRST_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_RXDIS;
    SECOND_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_RXDIS;
    // Now activate autotx
    FIRST_SPI_USART->TRIGCTRL_SET = USART_TRIGCTRL_AUTOTXTEN;
    SECOND_SPI_USART->TRIGCTRL_SET = USART_TRIGCTRL_AUTOTXTEN;
    PRS->ASYNC_SWLEVEL = 1;
    //
    return address;
}

