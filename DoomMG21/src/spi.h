/**
 *  Spi low level functions for of Doom Port on xMG21/Ikea Tradfri
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
 *  low level functions to set pin directions and functions.
 *
 */
#ifndef SRC_SPI_H_
#define SRC_SPI_H_

#include <stdint.h>
#include "em_device.h"
#include "main.h"

static inline uint8_t spiread(uint8_t cmd)
{
    FIRST_SPI_USART->CMD_SET = USART_CMD_TXEN | USART_CMD_RXEN;
    while (!(FIRST_SPI_USART->STATUS & USART_STATUS_TXBL));
    FIRST_SPI_USART->TXDATA = cmd;
    while (!(FIRST_SPI_USART->STATUS & USART_STATUS_RXDATAV));
    uint8_t data = FIRST_SPI_USART->RXDATA;
    FIRST_SPI_USART->CMD_SET = USART_CMD_TXDIS | USART_CMD_CLEARRX | USART_CMD_RXDIS;
    return data;
}
static inline void setSingleSPI(void)
{
    // disable other SPI
    GPIO->USARTROUTE[SECOND_SPI_NUMBER].ROUTEEN = 0;
    // set port I/O
#if WSTKBOARD
    GPIO->P[SPI_PORT].MODEL = (gpioModePushPullAlternate << (SPI_MOSI_PIN * 4)) | (gpioModeInputPull << (SPI_MISO_PIN * 4)) | (gpioModePushPullAlternate << (SPI_CLK_PIN * 4)) | (gpioModePushPull << (DISPLAY_NCS_PIN * 4)) | (gpioModePushPull << (DISPLAY_DC_PIN * 4)) | (gpioModePushPull << (FLASH_NCS_PIN * 4));
#else
    GPIO->P[SPI_PORT].MODEL = (gpioModePushPullAlternate << (SPI_MOSI_PIN * 4))
    | (gpioModeInputPull << (SPI_MISO_PIN * 4))
    | (gpioModePushPullAlternate << (SPI_CLK_PIN * 4))
    | (gpioModePushPull << (DISPLAY_DC_PIN * 4))
    | (gpioModeInputPull << (SR_OUT_PIN * 4))
    | (gpioModePushPull << (FLASH_NCS_PIN * 4));
#endif
    // we use only the first SPI
    GPIO->USARTROUTE[FIRST_SPI_NUMBER].TXROUTE = ((uint32_t) SPI_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) | (SPI_MOSI_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
    GPIO->USARTROUTE[FIRST_SPI_NUMBER].RXROUTE = ((uint32_t) SPI_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) | (SPI_MISO_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
    GPIO->USARTROUTE[FIRST_SPI_NUMBER].CLKROUTE = ((uint32_t) SPI_PORT << _GPIO_USART_CLKROUTE_PORT_SHIFT) | (SPI_CLK_PIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);
    GPIO->USARTROUTE[FIRST_SPI_NUMBER].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN | GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_CLKPEN;
    //
}
static inline void setDouleSpiOut(void)
{
    // all outputs
#if WSTKBOARD
    GPIO->P[SPI_PORT].MODEL = (gpioModePushPullAlternate << (SPI_MOSI_PIN * 4)) | (gpioModePushPullAlternate << (SPI_MISO_PIN * 4)) | (gpioModePushPullAlternate << (SPI_CLK_PIN * 4)) | (gpioModePushPull << (DISPLAY_NCS_PIN * 4)) | (gpioModePushPull << (DISPLAY_DC_PIN * 4)) | (gpioModePushPull << (FLASH_NCS_PIN * 4));
#else
    GPIO->P[SPI_PORT].MODEL = (gpioModePushPullAlternate << (SPI_MOSI_PIN * 4))
    | (gpioModePushPullAlternate << (SPI_MISO_PIN * 4))
    | (gpioModePushPullAlternate << (SPI_CLK_PIN * 4))
    | (gpioModeInputPull << (SR_OUT_PIN * 4))
    | (gpioModePushPull << (DISPLAY_DC_PIN * 4))
    | (gpioModePushPull << (FLASH_NCS_PIN * 4));
#endif
    // we use both SPI as outputs
    // First SPI MOSI routed to MOSI (DI, D0)
    GPIO->USARTROUTE[FIRST_SPI_NUMBER].TXROUTE = ((uint32_t) SPI_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) | (SPI_MOSI_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
    // Second SPI MOSI routed to MISO (DO, D1)
    GPIO->USARTROUTE[SECOND_SPI_NUMBER].TXROUTE = ((uint32_t) SPI_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) | (SPI_MISO_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
    // Enable only transmitters
    GPIO->USARTROUTE[SECOND_SPI_NUMBER].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN;
    GPIO->USARTROUTE[FIRST_SPI_NUMBER].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN | GPIO_USART_ROUTEEN_CLKPEN;
    //
}
static inline void setDoubleSpiIn(void)
{
    // all inputs
#if WSTKBOARD
    GPIO->P[SPI_PORT].MODEL = (gpioModeInputPull << (SPI_MOSI_PIN * 4)) | (gpioModeInputPull << (SPI_MISO_PIN * 4)) | (gpioModePushPullAlternate << (SPI_CLK_PIN * 4)) | (gpioModePushPull << (DISPLAY_NCS_PIN * 4)) | (gpioModePushPull << (DISPLAY_DC_PIN * 4)) | (gpioModePushPull << (FLASH_NCS_PIN * 4));
#else
    GPIO->P[SPI_PORT].MODEL = (gpioModeInputPull << (SPI_MOSI_PIN * 4))
    | (gpioModeInputPull << (SPI_MISO_PIN * 4))
    | (gpioModePushPullAlternate << (SPI_CLK_PIN * 4))
    | (gpioModeInputPull << (SR_OUT_PIN * 4))
    | (gpioModePushPull << (DISPLAY_DC_PIN * 4))
    | (gpioModePushPull << (FLASH_NCS_PIN * 4));
#endif
    // we use both SPI as input
    // First SPI MISO routed to MOSI (DI, D0)
    GPIO->USARTROUTE[FIRST_SPI_NUMBER].TXROUTE = ((uint32_t) SPI_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) | (6 << _GPIO_USART_RXROUTE_PIN_SHIFT);
    GPIO->USARTROUTE[FIRST_SPI_NUMBER].TXROUTE = ((uint32_t) SPI_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) | (6 << _GPIO_USART_RXROUTE_PIN_SHIFT);

    GPIO->USARTROUTE[FIRST_SPI_NUMBER].RXROUTE = ((uint32_t) SPI_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) | (SPI_MOSI_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
    // Second SPI MISI routed to MISO (DO, D1)
    GPIO->USARTROUTE[SECOND_SPI_NUMBER].RXROUTE = ((uint32_t) SPI_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) | (SPI_MISO_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
    //
    GPIO->USARTROUTE[SECOND_SPI_NUMBER].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN;
    GPIO->USARTROUTE[FIRST_SPI_NUMBER].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_CLKPEN;
    //
}
#endif /* SRC_SPI_H_ */
