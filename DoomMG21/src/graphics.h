/*
 * graphics.h
 *
 * Some graphics utilities
 */

#ifndef SRC_GRAPHICS_H_
#define SRC_GRAPHICS_H_
#include <stdint.h>
#include <stdbool.h>
void displayPrintf(int x, int y, const char *format, ...);
void drawScreen4bpp();
void setDisplayPen(int color, int background);
void clearScreen4bpp();
void displayPrintln(bool update, const char *format, ...);
#endif /* SRC_GRAPHICS_H_ */
