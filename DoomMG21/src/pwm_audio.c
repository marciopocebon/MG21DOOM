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
#include "pwm_audio.h"
#include "em_ldma.h"
#include "w_wad.h"
#include <string.h>
#include "sounds.h"
#include "i_spi_support.h"
//
int16_t *audioBuffer;

soundChannel_t soundChannels[MAX_CHANNELS];

static LDMA_Descriptor_t dmaXfer[1];  // 1 channel only

void initPwmAudio()
{
    // Set output
    GPIO->P[AUDIO_PORT].MODEL = (gpioModePushPullAlternate << (AUDIO_PIN * 4)) | (GPIO->P[AUDIO_PORT].MODEL & ~(0xF << (AUDIO_PIN * 4)));
    GPIO->P_CLR[AUDIO_PORT].DOUT = (1 << AUDIO_PIN);
    // TIMER 1 generates PWM
    TIMER1->CFG = TIMER_CFG_PRESC_DIV1 | TIMER_CFG_MODE_UPDOWN;
    TIMER1->CC[0].CFG = TIMER_CC_CFG_MODE_PWM;
    TIMER1->EN = TIMER_EN_EN;
    TIMER1->CC[0].CTRL = TIMER_CC_CTRL_ICEVCTRL_EVERYEDGE | TIMER_CC_CTRL_CMOA_TOGGLE;
    TIMER1->TOP = 0xFF;
    //
    TIMER1->CC[0].OC = 0x10;
    GPIO->TIMERROUTE[1].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
    GPIO->TIMERROUTE[1].CC0ROUTE = (AUDIO_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) | (AUDIO_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
    TIMER1->CMD = TIMER_CMD_START;
    // Configure Timer 2 to generate a signal every 1/11025 s
    TIMER2->CFG = TIMER_CFG_PRESC_DIV1 | TIMER_CFG_DMACLRACT;
    TIMER2->EN = TIMER_EN_EN;
    TIMER2->TOP = (80000000 / 11025) - 1;
    TIMER2->CMD = TIMER_CMD_START;
    //
    // reset DMA controller and set all channels as round-robin (NUMFIXED = 0)
    LDMA->CTRL = (0 << _LDMA_CTRL_NUMFIXED_SHIFT);
    LDMA->CHDIS = _LDMA_CHEN_MASK;
    LDMA->DBGHALT = 0;
    LDMA->REQDIS = 0;
    LDMA->EN = LDMA_EN_EN;
    // Config for looping sound
    LDMAXBAR->CH[0].REQSEL = LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER2 | LDMAXBAR_CH_REQSEL_SIGSEL_TIMER2UFOF;
    //
    LDMA->CH[0].LOOP = 0 << _LDMA_CH_LOOP_LOOPCNT_SHIFT;
    LDMA->CH[0].CFG = LDMA_CH_CFG_ARBSLOTS_ONE | LDMA_CH_CFG_SRCINCSIGN_POSITIVE | _LDMA_CH_CFG_DSTINCSIGN_POSITIVE;

    // configure transfer descriptor
    dmaXfer[0].xfer.structType = _LDMA_CH_CTRL_STRUCTTYPE_TRANSFER;
    // destination
    dmaXfer[0].xfer.dstAddrMode = _LDMA_CH_CTRL_DSTMODE_ABSOLUTE;
    dmaXfer[0].xfer.dstAddr = (uint32_t) &TIMER1->CC[0].OCB;
    dmaXfer[0].xfer.dstInc = _LDMA_CH_CTRL_DSTINC_NONE;
    //
    dmaXfer[0].xfer.srcAddrMode = _LDMA_CH_CTRL_SRCMODE_ABSOLUTE;
    dmaXfer[0].xfer.srcAddr = ((uint32_t) audioBuffer) + 1; //16 bit size, using only top part
    dmaXfer[0].xfer.srcInc = _LDMA_CH_CTRL_SRCINC_TWO;
    //
    dmaXfer[0].xfer.size = _LDMA_CH_CTRL_SIZE_BYTE;
    dmaXfer[0].xfer.blockSize = _LDMA_CH_CTRL_BLOCKSIZE_UNIT1; // one byte per transfer
    dmaXfer[0].xfer.xferCnt = AUDIO_BUFFER_LENGTH - 1;
    dmaXfer[0].xfer.linkAddr = (uint32_t) &dmaXfer[0] >> 2;
    dmaXfer[0].xfer.link = 1;
    dmaXfer[0].xfer.linkMode = _LDMA_CH_LINK_LINKMODE_ABSOLUTE;
    //
    /* Set the descriptor address. */
    LDMA->CH[0].LINK = ((uint32_t) &dmaXfer[0] & _LDMA_CH_LINK_LINKADDR_MASK) | LDMA_CH_LINK_LINK;
    //
    LDMA->IF_CLR = 1;
    //
    BUS_RegMaskedClear(&LDMA->CHDONE, 1); /* Clear the done flag.     */
    LDMA->LINKLOAD = 1; /* Start a transfer by loading the descriptor.  */
    //
    memset(soundChannels, 0, sizeof(soundChannels));
}
/**
 * Immediately shuts off sound
 */
void muteSound()
{
    for (int i = 0; i < AUDIO_BUFFER_LENGTH; i++)
    {
        audioBuffer[i] = ZERO_AUDIO_LEVEL;
    }
    for (int ch = 0; ch < MAX_CHANNELS; ch++)
    {
        soundChannels[ch].sfxIdx = 0;
        soundChannels[ch].volume = 0;
    }
}
/*
 * next-hack: poor's man mixer.
 *
 * There are N channels, each one can play one sample. We cannot have infinite buffer
 * and we cannot use interrupts, because we cannot interrupt time critical DSPI flash readout.
 * Therefore we create a buffer with 1024 samples, which is sent by DMA to the PWM.
 * Each sample of the buffer is 16-bit, because we need to mix all the channels.
 * The audio buffer is updated after all drawing operations have been done.
 *
 * Since doom's sample rate is 11025 Hz, then with a 1024 buffer, the minimum
 * required frame rate is about 11 fps, which is quite low and already unplayable on its own.
 *
 * The actual frame rate is unknown so we need to see where is the DMA source pointer, and
 * start updating some samples after its current position.
 *
 * This means a small delay for new samples, of some ms.
 *
 *
 */
void updateSound()
{
    // where are we in our circular buffer?
    uint32_t currentIdx = (LDMA->CH[0].SRC - (uint32_t) audioBuffer) / sizeof(audioBuffer[0]);
    // we cannot start updating the audio buffer just on next sample (which will occur in about 90us from now).
    // if frame rate is high enough, then the audio buffer is valid several samples after currentIdx.
    // therefore we can start updating the audio buffer AUDIO_BUFFER_DELAY samples after the current one.
    // AUDIO_BUFFER_DELAY must be chosen so that we have enough time to read the sample data from all the lumps
    // and store to the buffer. This might take some time.
    // The index at which we recalculate/update audio samples is startIdx.
    uint32_t startIdx = (currentIdx + AUDIO_BUFFER_DELAY) & (AUDIO_BUFFER_LENGTH - 1);
    // The buffer is recalculated from startIdx up to the sample before currentIdx
    // As first thing, zeroize buffer;
    for (int i = startIdx; i != ((currentIdx - 1) & (AUDIO_BUFFER_LENGTH - 1));
            i = (i + 1) & (AUDIO_BUFFER_LENGTH - 1))
    {
        audioBuffer[i] = ZERO_AUDIO_LEVEL;
    }
    // Now, for each channel we need to copy sample data.
    for (int ch = 0; ch < MAX_CHANNELS; ch++)
    {
        // channel active?
        if (soundChannels[ch].volume && soundChannels[ch].sfxIdx)
        {
            // get lump
            int lumpNum = p_wad_immutable_flash_data->soundLumps[soundChannels[ch].sfxIdx]; // W_CheckNumForName(S_sfx[soundChannels[ch].sfxIdx].name);
            if (lumpNum == -1)
                continue;
            const void *lumpPtr = W_CacheLumpNum(lumpNum);
            if (!isOnExternalFlash(lumpPtr))
                continue;
            // get sample size
            int32_t size = W_LumpLength(lumpNum);
            uint32_t samplesPlayed;
            // is this a new sample (not played before? Then do not change the offset
            if (soundChannels[ch].lastAudioBufferIdx != 0xFFFF)
            {
                // otherwise, we need to calcualte how many sample do we have played since last call.
                // considering also that we are starting at startIdx.
                samplesPlayed = (startIdx - soundChannels[ch].lastAudioBufferIdx) & (AUDIO_BUFFER_LENGTH - 1);
                soundChannels[ch].offset = soundChannels[ch].offset + samplesPlayed;
            }
            // remember last index.
            soundChannels[ch].lastAudioBufferIdx = startIdx;
            // how many bytes do we need to read?
            int32_t sizeToRead = size - soundChannels[ch].offset;
            if (sizeToRead <= 0) // already outputted all samples? zeroize index and volume.
            {
                soundChannels[ch].sfxIdx = 0;
                soundChannels[ch].volume = 0;
                continue;
            }
            // if the number of samples to read exceed the size of the buffer we need to update
            // then crop it
            if (sizeToRead > AUDIO_BUFFER_LENGTH - AUDIO_BUFFER_DELAY)
            {
                sizeToRead = AUDIO_BUFFER_LENGTH - AUDIO_BUFFER_DELAY;
            }
            // create a temporary buffer. In stack is ok, we have 1.5kB
            uint8_t tmpBuffer[AUDIO_BUFFER_LENGTH - AUDIO_BUFFER_DELAY];
            spiFlashSetAddress((uint32_t) lumpPtr + soundChannels[ch].offset);
            // read audio bytes
            spiFlashGetData(tmpBuffer, AUDIO_BUFFER_LENGTH - AUDIO_BUFFER_DELAY);
            //
            uint32_t stopIdx = (startIdx + sizeToRead) & (AUDIO_BUFFER_LENGTH - 1);
            //
            uint8_t *p = tmpBuffer;
            for (int i = startIdx; i != stopIdx;
                    i = (i + 1) & (AUDIO_BUFFER_LENGTH - 1))
            {
                // update audio buffer
                int16_t sampleValue = (0xFF & *p++) - 128;
                sampleValue *= soundChannels[ch].volume;
                audioBuffer[i] += sampleValue;
            }
        }
    }
}
