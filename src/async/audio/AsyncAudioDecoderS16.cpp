/**
@file	 AudioDecoderS16.cpp
@brief   An audio decoder for signed 16 bit samples
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-06

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdint.h>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioDecoderS16.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AudioDecoderS16::AudioDecoderS16(void)
{
  
} /* AudioDecoderS16::AudioDecoderS16 */


AudioDecoderS16::~AudioDecoderS16(void)
{
  
} /* AudioDecoderS16::~AudioDecoderS16 */


void AudioDecoderS16::writeEncodedSamples(void *buf, int size)
{
    // The "size" argument is derived from network input (e.g. the Reflector
    // UDP audio path passes the raw datagram payload length straight through).
    // The original implementation allocated a stack VLA, "float samples[size /
    // 2]", which a malicious peer could blow up into a stack overflow with a
    // large size, or trigger undefined behaviour with a negative size. Reject
    // implausible sizes here so a bad value never reaches an allocation, and
    // process the payload through a small fixed-size stack buffer instead of a
    // VLA. MAX_ENCODED_FRAME_SIZE is one second of mono 16-bit PCM at the
    // internal sample rate, which is far larger than any real audio frame
    // SvxLink emits but still a bounded, sane upper limit.
  static const int MAX_ENCODED_FRAME_SIZE =
      INTERNAL_SAMPLE_RATE * static_cast<int>(sizeof(int16_t));
  if ((size <= 0) || (size > MAX_ENCODED_FRAME_SIZE))
  {
    std::cerr << "*** WARNING: AudioDecoderS16 received an encoded frame with "
                 "an out of range size (" << size << " bytes). Discarding it."
              << std::endl;
    return;
  }

  int16_t *s16_samples = reinterpret_cast<int16_t *>(buf);
  int count = size / static_cast<int>(sizeof(int16_t));

    // Decode through a fixed-size stack buffer in chunks to avoid a VLA whose
    // size is controlled by the (now bounded) input.
  static const int CHUNK_SAMPLES = 512;
  float samples[CHUNK_SAMPLES];
  int pos = 0;
  while (pos < count)
  {
    int n = count - pos;
    if (n > CHUNK_SAMPLES)
    {
      n = CHUNK_SAMPLES;
    }
    for (int i=0; i<n; ++i)
    {
      samples[i] = static_cast<float>(s16_samples[pos+i]) / 32768.0;
    }
    sinkWriteSamples(samples, n);
    pos += n;
  }
} /* AudioDecoderS16::writeEncodedSamples */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

