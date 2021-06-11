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
#pragma GCC optimize ("Ofast")
#include "em_device.h"
#include "em_chip.h"
#include <stdint.h>
#include <stdbool.h>
#include "em_gpio.h"
#include "em_cmu.h"
#include "em_usart.h"
#include "mgm21_gpio.h"
#include "printf.h"
#include <string.h>
#include "em_ldma.h"
#include "ymodem.h"
#include "main.h"
#include "display.h"
#include "spi.h"
#include "spiFlashDual.h"
#include "z_zone.h"
#include "global_data.h"
#include "d_main.h"
#include "graphics.h"
#include "i_video.h"
#include "doom_iwad.h"
#include "i_system.h"
#include"pwm_audio.h"
//
#ifndef __STACK_SIZE
#warning __STACK_SIZE Not defined, 0x600 will be used.
#define __STACK_SIZE 0x600
#endif
#define UPLOAD_WAD_TEST_TIME_MS 2000UL
#define REBOOT_TIME_MS 2000UL
// Fetch CTUNE value from USERDATA page as a manufacturing token
#define MFG_CTUNE_ADDR 0x0FE00100UL
#define MFG_CTUNE_VAL  (*((uint16_t *) (MFG_CTUNE_ADDR)))
// <o SL_DEVICE_INIT_HFXO_CTUNE> CTUNE <0-255>
// <i> Default: 140
#define SL_DEVICE_INIT_HFXO_CTUNE          140
#define SL_DEVICE_INIT_HFXO_FREQ           38400000
#define SL_DEVICE_INIT_LFXO_MODE           cmuLfxoOscMode_Crystal
// <o SL_DEVICE_INIT_LFXO_CTUNE> CTUNE <0-127>
// <i> Default: 63
#define SL_DEVICE_INIT_LFXO_CTUNE          63
// <o SL_DEVICE_INIT_LFXO_PRECISION> LFXO precision in PPM <0-65535>
// <i> Default: 500
#define SL_DEVICE_INIT_LFXO_PRECISION      100
//
//
#define putchar _putchar
void initSystem()
{
    // set RC oscillator to run at 80MHz
    CMU_HFRCODPLLBandSet(cmuHFRCODPLLFreq_80M0Hz);
    //
    CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
    hfxoInit.mode = cmuHfxoOscMode_Crystal;
    //
    int ctune = -1;
    // Use HFXO tuning value from MFG token in UD page if not already set
    if ((MFG_CTUNE_VAL != 0xFFFF))
    {
        ctune = MFG_CTUNE_VAL;
    }
    //
    // Use HFXO tuning value from configuration header as fallback
    if (ctune == -1)
    {
        ctune = SL_DEVICE_INIT_HFXO_CTUNE;
    }
    //
    if (ctune != -1)
    {
        hfxoInit.ctuneXoAna = ctune;
        hfxoInit.ctuneXiAna = ctune;
    }
    // Configure external crystal oscillator
    SystemHFXOClockSet(SL_DEVICE_INIT_HFXO_FREQ);
    CMU_HFXOInit(&hfxoInit);
    //
    CMU_LFXOInit_TypeDef lfxoInit = CMU_LFXOINIT_DEFAULT;

    lfxoInit.mode = SL_DEVICE_INIT_LFXO_MODE;
    lfxoInit.capTune = SL_DEVICE_INIT_LFXO_CTUNE;

    CMU_LFXOInit(&lfxoInit);
    CMU_LFXOPrecisionSet(SL_DEVICE_INIT_LFXO_PRECISION);
    //
    // Enable radio clk to have access to SEQ and FCR ram.
    CMU->RADIOCLKCTRL = CMU_RADIOCLKCTRL_EN;
    SYSCFG->RADIORAMCTRL = SYSCFG_RADIORAMCTRL_FRCRAMPREFETCHEN | SYSCFG_RADIORAMCTRL_FRCRAMCACHEEN
     | SYSCFG_RADIORAMCTRL_FRCRAMWSEN // overclock if commented!
     | SYSCFG_RADIORAMCTRL_SEQRAMWSEN // overclock if commented!
    | SYSCFG_RADIORAMCTRL_SEQRAMCACHEEN | SYSCFG_RADIORAMCTRL_SEQRAMPREFETCHEN;

    // set clocks
    CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFRCODPLL);
    CMU_ClockSelectSet(cmuClock_PCLK, cmuSelect_HFRCODPLL);
    //
    CMU_ClockSelectSet(cmuClock_EM01GRPACLK, cmuSelect_HFRCODPLL);
    CMU_ClockSelectSet(cmuClock_EM23GRPACLK, cmuSelect_LFXO);
    CMU_ClockSelectSet(cmuClock_EM4GRPACLK, cmuSelect_LFXO);
    CMU_ClockSelectSet(cmuClock_RTCC, cmuSelect_LFXO);
    CMU_ClockSelectSet(cmuClock_WDOG0, cmuSelect_LFXO);
    //
#if WDOG_COUNT > 1
    CMU_ClockSelectSet(cmuClock_WDOG1, cmuSelect_LFXO);
#endif
    // enable RAM prefetch and cache
    SYSCFG->DMEM0RAMCTRL = SYSCFG_DMEM0RAMCTRL_RAMPREFETCHEN | SYSCFG_DMEM0RAMCTRL_RAMCACHEEN | SYSCFG_DMEM0RAMCTRL_RAMWSEN;
    // Note: peripheral clock overclock if divisor is 1 !
    CMU_ClockDivSet(cmuClock_PCLK, 1);
    // enable VCOM for debug messages.
    GPIO_PinModeSet(gpioPortD, 3, gpioModePushPull, 1);
    // VCOM UART: UART1 TX @ PA05, RX @ PA06
    GPIO_PinModeSet(gpioPortA, DBG_UART_TX_PIN, gpioModePushPullAlternate, 1);
    GPIO_PinModeSet(gpioPortA, DBG_UART_RX_PIN, gpioModeInputPull, 1);
    //
    GPIO->USARTROUTE[1].TXROUTE = (gpioPortA << _GPIO_USART_TXROUTE_PORT_SHIFT) | (DBG_UART_TX_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
    GPIO->USARTROUTE[1].RXROUTE = (gpioPortA << _GPIO_USART_RXROUTE_PORT_SHIFT) | (DBG_UART_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);

    //
    GPIO->USARTROUTE[1].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN | GPIO_USART_ROUTEEN_RXPEN;
    USART_InitAsync_TypeDef uart1Init = USART_INITASYNC_DEFAULT;
    USART_InitAsync(USART1, &uart1Init);
    // SPI config.
    USART_InitSync_TypeDef spiInit = USART_INITSYNC_DEFAULT;
    // modify default settings
    spiInit.baudrate = 40000000; // OVERCLOCK !
    spiInit.msbf = true;
    spiInit.enable = usartDisable; // do not enable now. We will enable using PRS
    // FIRST SPI
    USART_InitSync(FIRST_SPI_USART, &spiInit);
    // set usart to listen for enable from PRS
    FIRST_SPI_USART->TRIGCTRL_SET = USART_TRIGCTRL_TXTEN | USART_TRIGCTRL_RXTEN;
    //
    // SECOND SPI
    //
    // Set MISO AND MOSI LINES HIGH
    GPIO->P_SET[SPI_PORT].DOUT = (1 << FLASH_NCS_PIN) | (1 << DISPLAY_DC_PIN) | (1 << SPI_MISO_PIN) | (1 << SPI_MOSI_PIN);
    //
    USART_InitSync(SECOND_SPI_USART, &spiInit);
    // set usart to listen for enable from PRS
    SECOND_SPI_USART->TRIGCTRL_SET = USART_TRIGCTRL_TXTEN | USART_TRIGCTRL_RXTEN;
    // Activate delay to sample near next edge. This allows to go at higher speed.
    // Set also the buffer level so that we can send two bytes at once checking TXBL
    FIRST_SPI_USART->CTRL_SET = USART_CTRL_SMSDELAY | USART_CTRL_TXBIL;
    SECOND_SPI_USART->CTRL_SET = USART_CTRL_SMSDELAY | USART_CTRL_TXBIL;
    // SET PRS to trigger both USART at the same time
    PRS->CONSUMER_USART2_TRIGGER = 0;   // channel 0
    PRS->CONSUMER_USART0_TRIGGER = 0;   // channel 0
    // Set IRQ on data valid. This is not actually used for IRQs. This is used together
    // with WFI, which is much faster than checking the ISR bits.
    SECOND_SPI_USART->IEN = USART_IEN_RXDATAV;
    //
    setSingleSPI();
    //
    // Set the timer, clock at 10 MHz
    TIMER0->CFG = TIMER_CFG_PRESC_DIV8;
    TIMER0->EN = TIMER_EN_EN;
    TIMER0->TOP = 0xFFFFFFFF;
    TIMER0->CMD = TIMER_CMD_START;
    // Set Gpio strength for fast SPI
    GPIO_SlewrateSet(gpioPortC, 7, 7);
#if WSTKBOARD
    // Set Shift register data pin as alternate input
    GPIO->P[SR_OUT_PORT].MODEL = (gpioModeInputPull << (SR_OUT_PIN * 4)) | (GPIO->P[SR_OUT_PORT].MODEL & ~(0xF << (SR_OUT_PIN * 4)));
#else
  GPIO->P[DISPLAY_NCS_PORT].MODEL =  (gpioModePushPull << (DISPLAY_NCS_PIN * 4)) | (GPIO->P[DISPLAY_NCS_PIN].MODEL & ~(0xF << (DISPLAY_NCS_PIN * 4)));

#endif
}

void _putchar(char c)
{
    USART_Tx(USART1, c);
}

int main(void)
{
    // stack canary for debug.
    for (int i = 0x20018000 - __STACK_SIZE; i < 0x20018000; i += 4)
        *((uint32_t*) i) = 0xDEADBEEF;
    /* Chip errata */
    CHIP_Init();
    // init clock, peripheral, etc.
    initSystem();
    //
    printf("Up and running!\r\n");
    // put the bus speed to non overclocked mode. This is required, because we need to set the flash drive strength
    CMU_ClockDivSet(cmuClock_PCLK, 2);
    disableSpiFlashDualContinuousReadMode();
    setSingleSPI();
    spiFlashDriveStrength();
    // back to full speed!
    CMU_ClockDivSet(cmuClock_PCLK, 1);
    // deselect flash.
    FLASH_NCS_HIGH();
    // enable fault handlers (for debug)
    SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_USGFAULTENA_Msk;
    // Set SPI SPEED for display mode
    // low speed: 30 MHz
    FIRST_SPI_USART->CLKDIV = 128;
    //
    DisplayInit();
    BeginUpdateDisplay();
    for (int i = 0; i < 128 * 160; i++)
    {
        uint16_t color = 0xFF00;
        spiread(color);
        spiread(color >> 8);
    }
    //
    uint32_t time = I_GetTimeDeciMicrosecs();
    setDisplayPen(2, 0);

    displayPrintln(1, "Doom port to IKEA");
    displayPrintln(1, "TRADFRI RGB GU 10");
    displayPrintln(1, "Lamp LED1923R5.");
    displayPrintln(1, "");
    displayPrintln(1, "Flash size %d", spiFlashGetSize());
    setDisplayPen(3, 0);
    displayPrintln(1, "Press USE, CHG, ALT");
    displayPrintln(1, "to upload a new WAD");
    //
    while (I_GetTimeDeciMicrosecs() - time < UPLOAD_WAD_TEST_TIME_MS * 10000);
    //

    //
    DISPLAY_NCS_HIGH();
    //
    // Back to full SPI speed (40MHz)
    FIRST_SPI_USART->CLKDIV = 0;
    setDisplayPen(2, 1);
    if (keysDown() == (KEY_ALT | KEY_CHGW | KEY_USE))
    {

        displayPrintln(1, "Y modem lauch");
        ymodemReceive((uint32_t) doom_iwad);
        time = I_GetTimeDeciMicrosecs();
        displayPrintln(1, "Rebooting in 2 sec!");
        while (I_GetTimeDeciMicrosecs() - time < REBOOT_TIME_MS * 10000);
    }
    else
    {
        displayPrintln(1, "Starting Doom!");
    }
    //
    FLASH_NCS_HIGH();
    //
    setSpiFlashDualContinuousReadMode(EXT_FLASH_BASE);
    //
    //
    FLASH_NCS_LOW();
    //
    Z_Init(); /* 1/18/98 killough: start up memory stuff first */
    //
    InitGlobals();
    //
    D_DoomMain();

}

