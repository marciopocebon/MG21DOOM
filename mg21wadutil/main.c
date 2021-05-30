/*
   MG21Wadutil by Nicola Wrachien.

   Based on doomhack's GBAWadutil
   Original source: https://github.com/doomhack/GbaWadUtil

   This command line utility allows to convert your wad file
   to a format convenient for the Ikea Tradfri / MG21 Doom port.

   Usage:
   mg21wadutil <inputwad> <outputwad>
*/
#include <stdio.h>
#include <stdlib.h>
#include "wadfile.h"
#include "wadprocessor.h"

int main(int argc, char *argv[])
{
    wadfile_t wadfile;
    wadfile_t gbawadfile;
    printf("MG21wadutil by Nicola Wrachien.\r\nOriginal source by doomhack.\r\n");
#if DEBUG_MODE == 0
    if (argc != 3)
    {
        printf("Usage: %s <input wad> <output wad>\r\n", argv[0]);
        return 0;
    }
    loadWad(argv[1], &wadfile );
    processWad(&wadfile, false);
    loadWad("gbadoom.wad", &gbawadfile);
    mergeWadFile(&wadfile, &gbawadfile);
    saveWad(argv[2], &wadfile, 'I');
#else
    loadWad("doom1.wad", &wadfile );
    processWad(&wadfile, false);
    loadWad("gbadoom.wad", &gbawadfile);
    mergeWadFile(&wadfile, &gbawadfile);
    saveWad("doomCopyfull1.wad", &wadfile, 'I');
#endif
    return 0;
}
