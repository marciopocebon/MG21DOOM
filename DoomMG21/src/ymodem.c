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
 *  Poor man's YMODEM. Need serious cleanup
 *
 */
#include "em_device.h"
#include "printf.h"
#include "ymodem.h"
#include "main.h"
#include "spi.h"
#include "spiFlashDual.h"
#include "display.h"
#include "graphics.h"
//
#define XMODEM_SOH 0x01
#define XMODEM_SOX 0x02   // for 1k packets
#define XMODEM_EOT 0x04
#define XMODEM_ETB 0x17
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_ETB 0x17
#define XMODEM_CAN 0x18
#define XMODEM_C 0x43
#define MAXRETRANS 16
#define ONE_SECOND 10000000UL
#define putchar _putchar
//
extern uint8_t staticZone[]; // until Doom hasn't started, we can do everything we want on this buffer.
//
int getCharWithTimeout(uint32_t timeout)
{
    int c = -1;
    uint32_t timeNow = TIMER0->CNT;
    while ((uint32_t) TIMER0->CNT - timeNow < timeout)
    {
        if (USART1->IF & USART_IF_RXDATAV)
        {
            c = (uint8_t) USART1->RXDATA;
            break;
        }
    }
    return c;
}
//
static bool isCrcValid(uint8_t *buffer, int length)
{

    // compute CRC. We use hardware CRC functionality for this one.
    for (int i = 0; i < length; i++)
        GPCRC->INPUTDATABYTE = buffer[i];
    uint16_t data = GPCRC->DATA;
    if (data == 0)
    {
        return true;
    }
    else
    {
        displayPrintln(1, "CRC ERROR");
        displayPrintln(1, "r 0x%02x%02x c 0x%04x", buffer[length - 1], buffer[length - 2], data);
        return false;
    }
}
//
void verify(uint32_t address, uint8_t *buffer, int len)
{
    if (!len)
        return;
    FLASH_NCS_HIGH();
    setSpiFlashDualContinuousReadMode(address);
    for (int i = 0; i < len; i++)
    {
        uint8_t b = spiFlashGetByteDual();
        if (b != buffer[i])
        {
            displayPrintln(1, "i: %x", i);
            putchar(XMODEM_CAN);
            putchar(XMODEM_CAN);
            putchar(XMODEM_CAN);
            printf("Error, data not written correctly at address 0x%08x, got %d expected %d aborting\r\n", address, b, buffer[i]);
            disableSpiFlashDualContinuousReadMode();
            setSingleSPI();
            displayPrintln(1, "Err @0x%08x", address + i);
            displayPrintln(1, "Expected: 0x%02x", buffer[i]);
            displayPrintln(1, "Got: 0x%02x", b);
            while (1);
        }
    }
    disableSpiFlashDualContinuousReadMode();
    setSingleSPI();
}
//
static int str2int(uint8_t *str)
{
    int n = 0;
    while (*str != 0)
    {
        n = n * 10 + *str - '0';
        str++;
    }
    return n;
}
//
int ymodemReceive(uint32_t address)
{
    bool firstPacket = true;
    uint8_t erased = false;
    //unsigned char packet[1 + 2 + 1024 + 2]; // start of header, packet number, data, CRC16
    unsigned char *packet = staticZone;
    unsigned char sendChar = XMODEM_C;  // character to send
    unsigned char packetNumber = 1;
    int sizeOfPacket = 0;
    int c;
    int fileLength = 0xFFFFFF;    // 16MB
    int bytesProgrammed = 0;
    int packetRetry = MAXRETRANS;
    bool startReceive;

    GPCRC->CTRL = GPCRC_CTRL_AUTOINIT | GPCRC_CTRL_POLYSEL_CRC16 | GPCRC_CTRL_BITREVERSE_REVERSED;
    GPCRC->INIT = 0;
    GPCRC->POLY = __RBIT(0x10210000);
    GPCRC->EN = GPCRC_EN_EN;
    GPCRC->CMD = GPCRC_CMD_INIT;
    //
    //spiFlashChipErase();
    printf("Waiting for X or YMODEM transmission\r\n");
    while (1)
    {
        startReceive = false;
        for (int retry = 0; retry < 16; retry++)
        {
            // if we need to send a char, send it
            if (sendChar)
            {
                putchar(sendChar);
            }
            // did we get a character within 1 s?
            if ((c = getCharWithTimeout(3 * ONE_SECOND)) >= 0)
            {
                switch (c)
                {
                    case XMODEM_SOH:
                        startReceive = true;
                        sizeOfPacket = 1 + 2 + 128 + 2;
                        break;
                    case XMODEM_SOX:    // 1k modem support
                        startReceive = true;
                        sizeOfPacket = 1 + 2 + 1024 + 2;
                        break;
                    case XMODEM_ETB:
                    case XMODEM_EOT:
                        putchar(XMODEM_ACK);
                        //
                        putchar(XMODEM_ACK);
                        putchar(XMODEM_ACK);
                        putchar(XMODEM_ACK);
                        printf("File received, end of transmission.\r\n");
                        return 1;
                        break;
                    case XMODEM_CAN:
                        printf("Cancelled, rebooting\r\n");
                        NVIC_SystemReset();
                        break;
                    default:
                        displayPrintln(1, "Invalid char %d", c);
                        while (getCharWithTimeout(ONE_SECOND) >= 0);
                        putchar(XMODEM_NAK);
                        break;
                }
                if (startReceive)
                {
                    break;
                }
            }

        }
        if (startReceive)
        {
            // did we need to start from 0
            bool valid = true;
            packet[0] = c;      // SOH or SOX
            sendChar = 0;
            for (int i = 1; i < sizeOfPacket; i++)
            {
                c = getCharWithTimeout(3 * ONE_SECOND);
                if (c < 0)
                {
                    valid = false;
                    displayPrintln(1, "timeout %d, %d", packetNumber, i);
                    break;
                }
                packet[i] = c;
            }
            // check packet number field
            valid = valid && (packet[1] == (unsigned char) (~packet[2]));
            /*            if (!valid)
             {
             displayPrintln(1, "invalid %x %x %x %x", packetNumber, packet[0], packet[1], packet[2]);
             displayPrintln(1, "%x %x %x %x", packet[3], packet[4], packet[5] ,packet[6]);
             displayPrintln(1, "%x %x %x %x", packet[7], packet[8], packet[9] ,packet[10]);

             }
             else
             {
             //              displayPrintln(1, "valid %d", packetNumber);
             }*/
            // check if we are aligned
            valid = valid && ((packet[1] == packetNumber) || (!erased && (packet[1] == 0)));
            /*           if (!valid)
             {
             displayPrintln(1, "invalid %d", packetNumber);
             }
             else
             {
             //displayPrintln(1, "valid %d", packetNumber);
             }*/
            // check CRC
            valid = valid && isCrcValid(&packet[3], sizeOfPacket - 3);
            if (valid)
            {
                if (!erased) // this also means packet 0, which contains file name and size
                {
                    if (packet[1] == 0) // special case to support y modem
                    {
                        packetNumber = 0;
                        sizeOfPacket = 5; // so that this will result in a 0 bytesToProgram
                        // skip file name
                        uint8_t *p = &packet[3];
                        while (*p++ != 0);
                        // now get file size string
                        int idx = 0;
                        while (p[idx] != ' ' && idx < 8) // 8 digit size is 100M
                            idx++;
                        // found space.
                        p[idx] = 0;
                        fileLength = str2int(p);
                        displayPrintln(1, "File Length %d", fileLength);
                        sendChar = XMODEM_C;  // required for YMODEM
                    }
                    spiFlashChipErase();
                    erased = true;
                }
                //
                int bytesToProgram = sizeOfPacket - 5; // remove Start of header, packet number (2), and CRC (2)
                if (bytesToProgram > (fileLength - bytesProgrammed))
                {
                    bytesToProgram = fileLength - bytesProgrammed;
                }
                if (packetNumber % 8 == 0)
                    displayPrintln(1, "Prg %d %d%%", bytesProgrammed, 100 * bytesProgrammed / fileLength);
                spiFlashProgramForDualRead(address, &packet[3], bytesToProgram);
                // verify
                //
                verify(address, packet + 3, bytesToProgram);
                address += bytesToProgram;
                bytesProgrammed += bytesToProgram;
                //
                packetNumber++;
                // TODO: check overflow
                packetRetry = MAXRETRANS + 1;

                putchar(XMODEM_ACK);
                if (firstPacket)
                {
                    firstPacket = 0;
                    USART1->CMD = USART_CMD_CLEARRX;
                }
            }
            else
            {
                // before nacking we need to purge the line
                while (getCharWithTimeout(ONE_SECOND) >= 0);
                putchar(XMODEM_NAK);
            }
            if (--packetRetry <= 0)
            {
                putchar(XMODEM_CAN);
                putchar(XMODEM_CAN);
                putchar(XMODEM_CAN);
                printf("Too many packet errors, resetting\r\n");
                NVIC_SystemReset();
            }
        }
        else
        {
            printf("No start of frame \r\n");
        }
    }
    return 0;
}

