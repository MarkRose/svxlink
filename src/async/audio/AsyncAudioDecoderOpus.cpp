/**
@file	 AudioDecoderOpus.cpp
@brief   An audio decoder that use the Opus audio codec
@author  Tobias Blomberg / SM0SVX
@date	 2013-10-12

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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
#include <cstdlib>
#include <cmath>


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

#include "AsyncAudioDecoderOpus.h"



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

AudioDecoderOpus::AudioDecoderOpus(void)
  : frame_size(0)
{
  int error;
  dec = opus_decoder_create(INTERNAL_SAMPLE_RATE, 1, &error);
  if (error != OPUS_OK)
  {
    cerr << "*** ERROR: Could not initialize Opus decoder\n";
    exit(1);
  }
} /* AudioDecoderOpus::AudioDecoderOpus */


AudioDecoderOpus::~AudioDecoderOpus(void)
{
  opus_decoder_destroy(dec);
} /* AudioDecoderOpus::~AudioDecoderOpus */


void AudioDecoderOpus::setOption(const std::string &name,
      	      	      	      	  const std::string &value)
{
#if OPUS_MAJOR > 0
  if (name == "GAIN")
  {
    setGain(atof(value.c_str()));
  }
  else
#endif
  {
    cerr << "*** WARNING AudioDecoderOpus: Unknown option \""
      	 << name << "\". Ignoring it.\n";
  }
} /* AudioDecoderOpus::setOption */


void AudioDecoderOpus::printCodecParams(void) const
{
#if OPUS_MAJOR > 0
  cout << "------ Opus decoder parameters ------\n";
  cout << "Gain       = " << gain() << "dB\n";
  cout << "--------------------------------------\n";
#endif
} /* AudioDecoderOpus::printCodecParams */


#if OPUS_MAJOR > 0
float AudioDecoderOpus::setGain(float new_gain)
{
  opus_int32 gaini = static_cast<opus_int32>(256.0f * new_gain);
  opus_decoder_ctl(dec, OPUS_SET_GAIN(gaini));
  return gain();
} /* AudioDecoderOpus::setGain */


float AudioDecoderOpus::gain(void) const
{
  opus_int32 gaini;
    // coverity[ptr_arith]
  opus_decoder_ctl(dec, OPUS_GET_GAIN(&gaini));
  return gaini / 256.0f;
} /* AudioDecoderOpus::gain */
#endif


void AudioDecoderOpus::reset(void)
{
  opus_decoder_ctl(dec, OPUS_RESET_STATE);
} /* AudioDecoderOpus::reset */


void AudioDecoderOpus::writeEncodedSamples(void *buf, int size)
{
    // The "size" argument is the length of an Opus packet taken from network
    // input (e.g. the Reflector UDP audio path passes the datagram payload
    // length straight through). The original implementation derived a stack
    // VLA, "float samples[frame_cnt*frame_size]", from fields the attacker
    // controls inside the packet, and also passed the unchecked "size" to
    // opus_decode_float(). A large size could blow the stack and a negative
    // size is undefined behaviour. Reject implausible packet sizes up front and
    // decode into a fixed-size stack buffer, always telling Opus the true
    // capacity so it can never write past the buffer.
    //
    // MAX_ENCODED_FRAME_SIZE matches the encoder side's output buffer
    // (output_buf[4000] in AudioEncoderOpus), which is the size recommended by
    // the Opus documentation as an upper bound for a single packet.
    // MAX_DECODED_SAMPLES is the largest number of mono samples a single Opus
    // packet can decode to: Opus limits a packet to 120 ms of audio, so at the
    // internal sample rate this is an upper bound that no valid packet exceeds.
  static const int MAX_ENCODED_FRAME_SIZE = 4000;
  static const int MAX_DECODED_SAMPLES = 120 * INTERNAL_SAMPLE_RATE / 1000;
  if ((size <= 0) || (size > MAX_ENCODED_FRAME_SIZE))
  {
    std::cerr << "*** WARNING: AudioDecoderOpus received an encoded frame with "
                 "an out of range size (" << size << " bytes). Discarding it."
              << std::endl;
    return;
  }

  unsigned char *packet = reinterpret_cast<unsigned char *>(buf);

  int frame_cnt = opus_packet_get_nb_frames(packet, size);
  if (frame_cnt == 0)
  {
    return;
  }
  else if (frame_cnt < 0)
  {
    cerr << "*** ERROR: Opus decoder error: " << opus_strerror(frame_size)
         << endl;
    return;
  }
  frame_size = opus_packet_get_samples_per_frame(packet, INTERNAL_SAMPLE_RATE);
  if (frame_size == 0)
  {
    return;
  }
  else if (frame_size < 0)
  {
    cerr << "*** ERROR: Opus decoder error: " << opus_strerror(frame_size)
         << endl;
    return;
  }
  int channels = opus_packet_get_nb_channels(packet);
  if (channels <= 0)
  {
    cerr << "*** ERROR: Opus decoder error: " << opus_strerror(channels)
         << endl;
    return;
  }
  else if (channels != 1)
  {
    cerr << "*** ERROR: Multi channel Opus packet received but only one "
            "channel can be handled\n";
    return;
  }
  //cout << "### frame_cnt=" << frame_cnt << " frame_size=" << frame_size;
    // A well-formed Opus packet decodes to at most MAX_DECODED_SAMPLES mono
    // samples. Guard against malformed packets whose advertised frame_cnt /
    // frame_size would exceed that, then decode into a fixed-size stack buffer,
    // passing the real buffer capacity to opus_decode_float() as the output
    // frame-size limit so it can never overrun the buffer.
  if ((frame_cnt * frame_size) > MAX_DECODED_SAMPLES)
  {
    std::cerr << "*** WARNING: AudioDecoderOpus packet advertises more samples "
                 "than a valid Opus packet can contain. Discarding it."
              << std::endl;
    return;
  }
  float samples[MAX_DECODED_SAMPLES];
  frame_size = opus_decode_float(dec, packet, size, samples,
                                 MAX_DECODED_SAMPLES, 0);
  //cout << " " << frame_size << endl;
  if (frame_size > 0)
  {
    sinkWriteSamples(samples, frame_size);
  }
  else if (frame_size < 0)
  {
    cerr << "**** ERROR: Opus decoder error: " << opus_strerror(frame_size)
         << endl;
  }
} /* AudioDecoderOpus::writeEncodedSamples */



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

