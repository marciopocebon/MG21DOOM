/*
 *  Doom Port on xMG21/Ikea Tradfri
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
 *   next-hack:  On this port no multplayer is supported.
 *               Here we use the display-buffer (15kB) as stack, so we
 *               do not need to worry for stack overflows at least when dealing
 *               with Doom game-logic handling.
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "doomtype.h"
#include "doomstat.h"
#include "d_net.h"
#include "z_zone.h"

#include "d_main.h"
#include "g_game.h"
#include "m_menu.h"

#include "protocol.h"
#include "i_network.h"
#include "i_system.h"
#include "i_main.h"
#include "i_video.h"
#include "lprintf.h"

#include "global_data.h"

void D_InitNetGame(void)
{
    _g->playeringame = true;
}

void D_BuildNewTiccmds(void)
{
    int newtics = I_GetTime() - _g->lastmadetic;
    _g->lastmadetic += newtics;

    while (newtics--)
    {
        I_StartTic();
        if (_g->maketic - _g->gametic > 3)
            break;

        G_BuildTiccmd(&_g->netcmd);
        _g->maketic++;
    }
}

void TryRunTics2(void)
{
    int runtics;
    int entertime = I_GetTime();

    // Wait for tics to run
    while (1)
    {
        D_BuildNewTiccmds();

        runtics = (_g->maketic) - _g->gametic;
        if (runtics <= 0)
        {
            if (I_GetTime() - entertime > 10)
            {
                M_Ticker();
                return;
            }
        }
        else
            break;
    }

    while (runtics-- > 0)
    {

        if (_g->advancedemo)
            D_DoAdvanceDemo();
        M_Ticker();
        G_Ticker();

        _g->gametic++;
    }
}
void TryRunTics(void)
{
    // "displayBuffer" is a very large buffer, unused for the entire time. Why not using it as stack ?
    __asm volatile
    (
            "STR SP, [%[newStack]], #-4\n\t"
            "MOV SP, %[newStack]\n\t"
            : : [newStack] "r" (displayBuffer + sizeof(displayBuffer) - 8)
    );
    // actually run doom game
    TryRunTics2();
    // restore stack
    __asm volatile
    (
            "LDR SP, [%[newStack]]\n\t"
            : : [newStack] "r" (displayBuffer + sizeof(displayBuffer) - 4)
    );

}
