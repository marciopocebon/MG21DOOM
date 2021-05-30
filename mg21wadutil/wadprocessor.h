/*
   MG21Wadutil by Nicola Wrachien.

   Based on doomhack's GBAWadutil
   Original source: https://github.com/doomhack/GbaWadUtil

   This command line utility allows to convert your wad file
   to a format convenient for the Ikea Tradfri / MG21 Doom port.

   Usage:
   mg21wadutil <inputwad> <outputwad>
*/
#ifndef WADPROCESSOR_H
#define WADPROCESSOR_H

#include "wadfile.h"
bool processWad(wadfile_t *wadfile, bool removeSound);
#endif // WADPROCESSOR_H
