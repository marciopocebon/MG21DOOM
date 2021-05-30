/**
 *  Poor man's PWM audio driver for Doom Port on xMG21/Ikea Tradfri lamp.
 *
 *  Copyright (C) 2021 by Nicola Wrachien (next-hack in the comments)
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
 *  Simple audio driver. It supports multiple channels, but updateSound()
 *  shall be called frequently, otherwise sound glitches will occur.
 *  With a 1024 samples audio buffer, the minimum frame rate (and update rate)
 *  is  10.8 fps (11025 / 1024).
 *
 */
#ifndef SRC_PWM_AUDIO_H_
#define SRC_PWM_AUDIO_H_
#include "main.h"
#include "em_device.h"
#include "em_gpio.h"
#include "mgm21_gpio.h"
#define MAX_CHANNELS 8
#define AUDIO_BUFFER_LENGTH 1024
#define AUDIO_BUFFER_DELAY 25
#define ZERO_AUDIO_LEVEL 128*256
//
typedef struct
{
    uint16_t lastAudioBufferIdx;
    uint16_t offset;
    uint8_t sfxIdx;
    int8_t volume;
} soundChannel_t;
//
void initPwmAudio();
void updateSound();
void muteSound();
//
extern int16_t *audioBuffer;
extern soundChannel_t soundChannels[MAX_CHANNELS];

#endif /* SRC_PWM_AUDIO_H_ */
