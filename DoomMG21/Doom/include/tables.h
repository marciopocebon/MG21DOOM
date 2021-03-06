/*  Doom Port on xMG21/Ikea Tradfri
 *  by Nicola Wrachien (next-hack in the comments)
 *
 *  This port is based on the excellent doomhack's GBA Doom Port.
 *  Several data structures and functions have been optimized to fit the
 *  96kB + 12kB memory of xMG21 devices. Z-Depth Light has been restored with almost
 *  no RAM consumption!
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 * DESCRIPTION:
 *      Lookup tables.
 *      Do not try to look them up :-).
 *      In the order of appearance:
 *
 *      int finetangent[4096]   - Tangens LUT.
 *       Should work with BAM fairly well (12 of 16bit,
 *      effectively, by shifting).
 *
 *      int finesine[10240]             - Sine lookup.
 *       Guess what, serves as cosine, too.
 *       Remarkable thing is, how to use BAMs with this?
 *
 *      int tantoangle[2049]    - ArcTan LUT,
 *        maps tan(angle) to angle fast. Gotta search.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __TABLES__
#define __TABLES__

#include "m_fixed.h"
#include "doomdef.h"
//
// Lighting constants.

#define LIGHTLEVELS       16
#define LIGHTSEGSHIFT      4
#define MAXLIGHTSCALE     48
#define LIGHTSCALESHIFT   12
#define MAXLIGHTZ        128
#define LIGHTZSHIFT       20

// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.

#define NUMCOLORMAPS 32
//
#define TABULATED_VALUES  1 // use resolution-dependent tabulated values for some arrays.
#define FINEANGLES              8192
#define FINEMASK                (FINEANGLES-1)

// 0x100000000 to 0x2000
#define ANGLETOFINESHIFT        19

// Binary Angle Measument, BAM.
#define ANG45   0x20000000
#define ANG90   0x40000000
#define ANG180  0x80000000
#define ANG270  0xc0000000
#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#define SLOPERANGE 2048
#define SLOPEBITS    11
#define DBITS      (FRACBITS-SLOPEBITS)

typedef unsigned angle_t;

// Load trig tables if needed
void R_LoadTrigTables(void);

// Effective size is 10240.
extern const fixed_t finesine[10240];

// Re-use data, is just PI/2 phase shift.
extern const fixed_t *const finecosine;

// Effective size is 4096.
extern const fixed_t finetangent[4096];

// Effective size is 2049;
// The +1 size is to handle the case when x==y without additional checking.

extern const angle_t tantoangle[2049];

#if TABULATED_VALUES
extern const short viewangletox[4096];
extern const angle_t xtoviewangle[SCREENWIDTH + 1];
extern const fixed_t yslope[SCREENHEIGHT];
extern const fixed_t distscale[SCREENWIDTH];
extern const uint8_t scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern const uint8_t zlight[LIGHTLEVELS][MAXLIGHTZ];
#else
extern int viewangletox[4096];
extern angle_t xtoviewangle[SCREENWIDTH + 1];
extern fixed_t yslope[SCREENHEIGHT];
extern fixed_t distscale[SCREENWIDTH];
#endif

extern const short screenheightarray[SCREENWIDTH_PHYSICAL];

extern const short negonearray[SCREENWIDTH_PHYSICAL];

extern const unsigned char translationtables[1023];

#endif
