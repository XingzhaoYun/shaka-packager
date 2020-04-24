// Copyright 2016 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// AC4 audio utility functions.

#ifndef PACKAGER_MEDIA_CODECS_AC4_AUDIO_UTIL_H_
#define PACKAGER_MEDIA_CODECS_AC4_AUDIO_UTIL_H_

#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace shaka {
namespace media {

/// Parse data from AC4Specific box and calculate AC4 channel config value based
/// on ETSI TS 103 192-2 V1.2.1 Digital Audio Compression (AC-4)
/// Standard E.5
/// @return false if there are parsing errors.
bool CalculateAC4ChannelConfig(const std::vector<uint8_t>& ac4_data,
                               uint32_t& channel_config);

/// Generate MPEG audio channel configuration scheme value based on 
/// ETSI TS 103 192-2 V1.2.1 Digital Audio Compression (AC-4)
/// Standard G.3.2 
/// @return false if there are parsing errors.
bool CalculateAC4ChannelConfigMpegValue(const std::vector<uint8_t>& ac4_data,
                                        uint32_t& mpeg_value);

/// Generate the AC-4 codec sting based on
/// ETSI TS 103 190-2, V1.2.1 Digital Audio Compression (AC-4)
/// Standard E.13
/// @return false if there are parsing errors.
bool GetAc4CodecString(const std::vector<uint8_t>& ac4_data,
					   std::string& codec_string);

/// Get the inforation of whether ths AC-4 stream is IMS or not
/// @return false if there are parsing errors.
bool GetAc4ImsFlag(const std::vector<uint8_t>& ac4_data, uint32_t& payload);
}  // namespace media
}  // namespace shaka

#endif  // PACKAGER_MEDIA_CODECS_AC4_AUDIO_UTIL_H_
