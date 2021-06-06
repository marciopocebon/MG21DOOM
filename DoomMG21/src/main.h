/**
 *  Main file of Doom Port on xMG21/Ikea Tradfri
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
 *  main file (initialization and check for x/ymodem download request)
 *
 */
#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include <stdint.h>
#include <stdbool.h>
#include "printf.h"
#include "em_device.h"
#include "em_gpio.h"
// pin, port and peripheral definitions
#define SPI_PORT gpioPortC
#define DEBUG_SETUP 0
#if DEBUG_SETUP
#define WSTKBOARD 1
#define START_MAP 1
#define SHOW_FPS true
#else
#define WSTKBOARD 0
#define START_MAP 1
#define SHOW_FPS false
#endif

#if WSTKBOARD
// audio
#define AUDIO_PORT gpioPortA
#define AUDIO_PIN 4
// display and flash
#define DISPLAY_DC_PORT gpioPortC
#define DISPLAY_NCS_PORT gpioPortC
#define DISPLAY_NCS_PIN 0
#define SPI_CLK_PIN 1
#define DISPLAY_DC_PIN 2
#define SPI_MOSI_PIN 3
#define FLASH_NCS_PIN 4
#define SPI_MISO_PIN 5
// debug port
#define DBG_UART_TX_PIN 5
#define DBG_UART_RX_PIN 6
// shift register port and pin
#define SR_OUT_PIN 3
#define SR_OUT_PORT gpioPortA
#else
// audio
#define AUDIO_PORT gpioPortD
#define AUDIO_PIN 0
// display and flash
#define DISPLAY_DC_PORT gpioPortC
#define DISPLAY_NCS_PORT gpioPortD
#define DISPLAY_NCS_PIN 1   // PD1
#define SPI_CLK_PIN 2       // PC2
#define DISPLAY_DC_PIN 3    //PC3
#define SPI_MOSI_PIN 0     //PC0
#define FLASH_NCS_PIN 4     //PC4
#define SPI_MISO_PIN 5      // PC5
// debug port
#define DBG_UART_TX_PIN 3
#define DBG_UART_RX_PIN 4
// shift register port and pin
#define SR_OUT_PIN 1    // PC1
#define SR_OUT_PORT gpioPortC

#endif
//
#define FIRST_SPI_NUMBER 0
#define SECOND_SPI_NUMBER 2
#define FIRST_SPI_USART USART0
#define SECOND_SPI_USART USART2
#define USART_TEST_IRQn USART2_RX_IRQn

//
#define FLASH_NCS_HIGH() GPIO->P_SET[SPI_PORT].DOUT = 1 << FLASH_NCS_PIN
#define FLASH_NCS_LOW()   GPIO->P_CLR[SPI_PORT].DOUT = 1 << FLASH_NCS_PIN

#endif /* SRC_MAIN_H_ */
