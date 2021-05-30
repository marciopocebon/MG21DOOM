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
#include "spi.h"
#include "display.h"
#include "delay.h"
#include "TFT_ILI9163C_registers.h"
#define ILI9163_INIT_DELAY 0x80
#define nop() __asm__ volatile("nop");
void DisplayWriteCommand(uint8_t value)
{
    DISPLAY_DC_LOW();
    DISPLAY_NCS_LOW();
    nop();
    delay(1);
    spiread(value);
    DISPLAY_NCS_HIGH();
}
void DisplayWriteData(uint8_t value)
{
    DISPLAY_DC_HIGH();
    DISPLAY_NCS_LOW();
    delay(1);
    spiread(value);
    DISPLAY_NCS_HIGH();
}

void InitDisplayGPIO(void)
{
    DISPLAY_NCS_HIGH();
    DISPLAY_DC_HIGH();
}
static void executeDisplayCommands(const uint8_t *cmds)
{
    // Executes a list of commands for the display.
    /* cmds array has the following structure:
     *  number of commands
     *  Command Code, number of data bytes, data bytes (if any), delay in ms (if number of bytes is in OR with 0x80)
     *  ...
     */

    uint8_t ms, numArgs, numCommands;
    //
    // Send initialization commands
    numCommands = *cmds++;            // Number of commands to follow
    while (numCommands--)                           // For each command...
    {
        DisplayWriteCommand(*cmds++);    // Read, issue command
        numArgs = *cmds++;        // Number of args to follow
        ms = numArgs & ILI9163_INIT_DELAY;   // If hibit set, delay follows args
        numArgs &= ~ILI9163_INIT_DELAY;         // Mask out delay bit
        while (numArgs--)                       // For each argument...
        {
            DisplayWriteData(*cmds++); // Read, issue argument
        }

        if (ms)
        {
            ms = *cmds++;     // Read post-command delay time (ms)
            delay((ms == 255 ? 500 : ms));
        }
    }
}
void DisplayInit(void)
{
    // Display initialization.
    // Note: this function DOES NOT initialize the SPI.
    // It just initializes the I/O ports for D/C and nCS and sends the initialization commands to the display
    /* The following array has the following structure:
     *  number of commands
     *  Command Code, number of data bytes, data bytes (if any), delay in ms (if number of bytes is in OR with 0x80)
     *  ...
     */
    static const uint8_t ILI9163_cmds[] =
    { 18,             // 18 commands follow
    CMD_SWRESET, 0 | ILI9163_INIT_DELAY, 255, // Software reset. This first one is needed because of the RC reset.
    CMD_SWRESET, 0 | ILI9163_INIT_DELAY, 100,       // Software reset
    CMD_SLPOUT, 0 | ILI9163_INIT_DELAY, 100,       // Exit sleep mode
    CMD_PIXFMT, 1, 0x05, // Set pixel format
    CMD_GAMMASET, 1, 0x04, // Set Gamma curve
    CMD_GAMRSEL, 1, 0x01, // Gamma adjustment enabled
    CMD_FRMCTR1, 2, 0xA, 0x14,  // Frame rate control 1
    CMD_DINVCTR, 1, 0x07,       // Display inversion
    CMD_PWCTR1, 2, 0x0A, 0x02, // Power control 1
    CMD_PWCTR2, 1, 0x02,       // Power control 2
    CMD_VCOMCTR1, 2, 0x4F, 0x5A, // Vcom control 1 (0x4F 0x5A)
    CMD_VCOMOFFS, 1, 0x40,       // Vcom offset
    CMD_CLMADRS, 4 | ILI9163_INIT_DELAY, 0x00, 0x00, 0x00, 0x9F, 250, // Set column address  0x00  0x00 0x00 0x9F, 0 (no delay)
    CMD_PGEADRS, 4, 0x00, 0x00, 0x00, 0x7F, // Set page address 0x00 0x00 0x00 0x7F
    CMD_MADCTL, 1, 0x60,       // Set address mode 0xC8
    CMD_DISPON, 0,             // Set display on
    };
    InitDisplayGPIO();
    executeDisplayCommands(ILI9163_cmds);
}
void BeginUpdateDisplay()
{
    static const uint8_t ILI9163_cmds[] =
    { 3,
    CMD_CLMADRS, 4, 0, 0, 0, 0x9F,
    CMD_PGEADRS, 4, 0, 0, 0, 127,
    CMD_RAMWR, 0 };
    executeDisplayCommands(ILI9163_cmds);
    delay(1);
    DISPLAY_DC_HIGH();
    delay(1);
    DISPLAY_NCS_LOW();
    delay(1);
    nop();
}

