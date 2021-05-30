/*
 *  This file contains display-specific functions.
 *
 *  Display:
 *  These functions support a 160x128 SPI display. Even though it is for ILI9163,
 *  this works for ST7735 too!
 *
 *  Original code: Adafruit tft library?
 *
 *  Modified by Nicola Wrachien (next-hack in the comments) 2018-2021
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program  is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef DISPLAYH
#define DISPLAYH

#ifdef __cplusplus
extern "C"
{
#endif
#include "em_device.h"
#include "main.h"
// Useful macros
#define DISPLAY_DC_HIGH()  GPIO->P_SET[DISPLAY_DC_PORT].DOUT = (1 << DISPLAY_DC_PIN)
#define DISPLAY_DC_LOW()  GPIO->P_CLR[DISPLAY_DC_PORT].DOUT = (1 << DISPLAY_DC_PIN)
#define DISPLAY_NCS_HIGH()  GPIO->P_SET[DISPLAY_NCS_PORT].DOUT = (1 << DISPLAY_NCS_PIN)
#define DISPLAY_NCS_LOW() GPIO->P_CLR[DISPLAY_NCS_PORT].DOUT = (1 << DISPLAY_NCS_PIN)

// screen size
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 128
#define MAXROWS (SCREEN_HEIGHT/8)
#define MAXCOLS (SCREEN_WIDTH/8)
void UpdateDisplay(void);
void DisplayInit(void);
void BeginUpdateDisplay(void);
void EndUpdateDisplay(void);
void SelectDisplay(void);
void DisplayWriteData(uint8_t value);
void initDisplaySpi();

#ifdef __cplusplus
}
#endif

#endif
