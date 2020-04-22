// Copyright 2016 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "packager/media/codecs/ac4_audio_util.h"

#include "packager/base/macros.h"
#include "packager/base/strings/string_number_conversions.h"
#include "packager/base/strings/stringprintf.h"
#include "packager/media/base/bit_reader.h"
#include "packager/media/base/rcheck.h"

namespace shaka {
namespace media {

namespace {

//mapping based on ETSI TS 103 190-2, Table G.1
int MappingChannelConfigtoMpegSchemeValue(uint32_t channelConfig) {
  int ret = 0;

  switch (channelConfig) {
    case 0x2:
      ret = 1;
      break;
    case 0x1:
      ret = 2;
      break;
    case 0x3:
      ret = 3;
      break;
    case 0x8003:
      ret = 4;
      break;
    case 0x7:
      ret = 5;
      break;
    case 0x47:
      ret = 6;
      break;
    case 0x20047:
      ret = 7;
      break;
    case 0x8001:
      ret = 9;
      break;
    case 0x5:
      ret = 10;
      break;
    case 0x8047:
      ret = 11;
      break;
    case 0x4f:
      ret = 12;
      break;
    case 0x2ff7f:
      ret = 13;
      break;
    case 0x6ff6f:
      ret = 13;
      break;
    case 0x57:
      ret = 14;
      break;
    case 0x40047:
      ret = 14;
      break;
    case 0x145f:
      ret = 15;
      break;
    case 0x4144f:
      ret = 15;
      break;
    case 0x77:
      ret = 16;
      break;
    case 0x40067:
      ret = 16;
      break;
    case 0xa77:
      ret = 17;
      break;
    case 0x40a67:
      ret = 17;
      break;
    case 0xa7f:
      ret = 18;
      break;
    case 0x40a6f:
      ret = 18;
      break;
    case 0x7f:
      ret = 19;
      break;
    case 0x4006f:
      ret = 19;
      break;
    case 0x1007f:
      ret = 20;
      break;
    case 0x5006f:
      ret = 20;
      break;
    default:
      ret = -1;
  }

  return ret;
}

bool ac4_substream_group_dsi(BitReader& bit_reader) {
  bool b_substream_present;
  bit_reader.ReadBits(1, &b_substream_present);
  bool b_hsf_ext;
  bit_reader.ReadBits(1, &b_hsf_ext);
  bool b_channel_coded;
  bit_reader.ReadBits(1, &b_channel_coded);
  uint8_t n_substreams = 0;
  bit_reader.ReadBits(8, &n_substreams);
  for (uint8_t i = 0; i < n_substreams; i++) {
    bit_reader.SkipBits(2);
    bool b_substream_bitrate_indicator;
    bit_reader.ReadBits(1, &b_substream_bitrate_indicator);
    if (b_substream_bitrate_indicator) {
      bit_reader.SkipBits(5);
    }
    if (b_channel_coded) {
      bit_reader.SkipBits(24);
    } else {
      bool b_ajoc;
      bit_reader.ReadBits(1, &b_ajoc);
      if (b_ajoc) {
        bool b_static_dmx;
        bit_reader.ReadBits(1, &b_static_dmx);
        if (b_static_dmx == 0) {
          bit_reader.SkipBits(4);
        }
        bit_reader.SkipBits(6);
      }
      bit_reader.SkipBits(4);
    }
  }
  bool b_content_type;
  bit_reader.ReadBits(1, &b_content_type);
  if (b_content_type) {
    bit_reader.SkipBits(3);
    bool b_language_indicator;
    bit_reader.ReadBits(1, &b_language_indicator);
    if (b_language_indicator) {
      uint8_t n_language_tag_bytes;
      bit_reader.ReadBits(6, &n_language_tag_bytes);
      bit_reader.SkipBits(n_language_tag_bytes * 8);
    }
  }
  return true;
}

inline bool byte_align(uint32_t cur_bits, BitReader& bit_reader) {
  if (cur_bits % 8) {
    bit_reader.SkipBits(8 - (cur_bits % 8));
  }
  return true;
}

bool ac4_presentation_v1_dsi(BitReader& bit_reader,
                             uint8_t& mdcompat,
                             uint32_t& presentation_channel_config,
                             uint32_t pres_bytes,
                             uint8_t& dolby_atmos_indicator) {
  const uint32_t presentation_start = bit_reader.bit_position();
  uint8_t presentation_config_v1;
  bit_reader.ReadBits(5, &presentation_config_v1);
  uint8_t b_add_emdf_substreams;
  if (presentation_config_v1 == 0x06) {
    b_add_emdf_substreams = 1;
  } else {
    bit_reader.ReadBits(3, &mdcompat);
    bool b_presentation_group_index;
    bit_reader.ReadBits(1, &b_presentation_group_index); // b_presentation_group_index
    if (b_presentation_group_index) {
      bit_reader.SkipBits(5); // presentation_group_index
    }
    bit_reader.SkipBits(19);
    bool b_presentation_channel_coded;
    bit_reader.ReadBits(1, &b_presentation_channel_coded);
    if (b_presentation_channel_coded) {
      uint8_t dsi_presentation_ch_mode;
      bit_reader.ReadBits(5, &dsi_presentation_ch_mode);
      if (dsi_presentation_ch_mode >= 11 && dsi_presentation_ch_mode <= 14) {
        bit_reader.SkipBits(3); // 
      }
      bit_reader.ReadBits(24, &presentation_channel_config);
    }
    bool b_presentation_core_differs;
    bit_reader.ReadBits(1, &b_presentation_core_differs);
    if (b_presentation_core_differs) {
      bool b_presentation_core_channel_coded;
      bit_reader.ReadBits(1, &b_presentation_core_channel_coded);
      if (b_presentation_core_channel_coded) {
        bit_reader.SkipBits(2);
      }
    }
    bool b_presentation_filter;
    bit_reader.ReadBits(1, &b_presentation_filter);
    if (b_presentation_filter) {
      bit_reader.SkipBits(1);
      uint8_t n_filter_bytes;
      bit_reader.ReadBits(8, &n_filter_bytes);
      bit_reader.SkipBits(n_filter_bytes * 8);
    }
    if (presentation_config_v1 == 0x1f) {
      // ac4_substream_group_dsi();
      ac4_substream_group_dsi(bit_reader);
    } else {
      bit_reader.SkipBits(1); // b_multi_pid
      if (presentation_config_v1 == 0 || presentation_config_v1 == 1 || presentation_config_v1 == 2) {
        // ac4_substream_group_dsi();
        // ac4_substream_group_dsi();
        ac4_substream_group_dsi(bit_reader);
        ac4_substream_group_dsi(bit_reader);

      }
      if (presentation_config_v1 == 3 || presentation_config_v1 == 4) {
        // ac4_substream_group_dsi();
        // ac4_substream_group_dsi();
        // ac4_substream_group_dsi();
        ac4_substream_group_dsi(bit_reader);
        ac4_substream_group_dsi(bit_reader);
        ac4_substream_group_dsi(bit_reader);
      }
      if (presentation_config_v1 == 5) {
        uint8_t n_substream_groups_minus2;
        bit_reader.ReadBits(3, &n_substream_groups_minus2);
        for (uint8_t i = 0; i < n_substream_groups_minus2 + 2; i++) {
          // ac4_substream_group_dsi();
          ac4_substream_group_dsi(bit_reader);
        }
      }
      if (presentation_config_v1 > 5) {
        uint8_t n_skip_bytes;
        bit_reader.ReadBits(7, &n_skip_bytes);
        bit_reader.SkipBits(n_skip_bytes * 8);
      }
    }
    bit_reader.SkipBits(1); //b_pre_virtualized
    bit_reader.ReadBits(1, &b_add_emdf_substreams);
  }
  if (b_add_emdf_substreams) {
    uint8_t n_add_emdf_substreams;
    bit_reader.ReadBits(7, &n_add_emdf_substreams);
    bit_reader.SkipBits(n_add_emdf_substreams * 15);
  }
  bool b_presentation_bitrate_info;
  bit_reader.ReadBits(1, &b_presentation_bitrate_info);
  if (b_presentation_bitrate_info) {
    // ac4_bitrate_dsi();
    bit_reader.SkipBits(66);
  }
  bool b_alternative;
  bit_reader.ReadBits(1, &b_alternative);
  if (b_alternative) {
    if (!byte_align(bit_reader.bit_position() - presentation_start, bit_reader)) {
      return false;
    }
    // alternative_info();
    uint16_t name_len;
    bit_reader.ReadBits(16, &name_len);
    bit_reader.SkipBits(name_len * 8);
    uint8_t n_targets;
    bit_reader.ReadBits(5, &n_targets);
    bit_reader.SkipBits(n_targets * 11);
  }
  if (!byte_align(bit_reader.bit_position() - presentation_start, bit_reader)) {
    return false;
  }
  if ((bit_reader.bit_position() - presentation_start) <= (pres_bytes - 1) * 8) {
    bit_reader.SkipBits(1);
    bit_reader.ReadBits(1, &dolby_atmos_indicator);
    bit_reader.SkipBits(4);
    bool b_extended_presentation_group_index;
    bit_reader.ReadBits(1, &b_extended_presentation_group_index);
    if (b_extended_presentation_group_index) {
      bit_reader.SkipBits(9);
    } else {
      bit_reader.SkipBits(1);
    }
  }
  return true;
}

bool ExtractAc4Data(const std::vector<uint8_t>& ac4_data,
  uint8_t& bitstream_version,
  uint8_t& presentation_version,
  uint8_t& is_ims,
  uint8_t& mdcompat,
  uint32_t& presentation_channel_config,
  uint8_t& dolby_atmos_indicator) {
  uint16_t n_presentation;

  RCHECK(ac4_data.size() > 0);
  BitReader bit_reader(ac4_data.data(), ac4_data.size());

  RCHECK(bit_reader.SkipBits(3) &&
    bit_reader.ReadBits(7, &bitstream_version));

  RCHECK(bit_reader.SkipBits(5) &&
    bit_reader.ReadBits(9, &n_presentation));

  if (bitstream_version > 1) {
    uint8_t b_program_id = 0;
    bit_reader.ReadBits(1, &b_program_id);
    if (b_program_id) {
      bit_reader.SkipBits(16);
      uint8_t temp = 0;
      bit_reader.ReadBits(1, &temp);
      if (temp) {
        for (uint8_t i = 0; i < 8; i++) {
          bit_reader.SkipBits(16);
        }
      }
    }
  } else if (bitstream_version == 0) {
    LOG(WARNING) << "Bitstream version 0 is not supported";
    return false;
  } else {
    LOG(WARNING) << "Invaild Bitstream version";
    return false;
  }

  bit_reader.SkipBits(66);

  if (!byte_align(bit_reader.bit_position(), bit_reader)) {
    return false;
  }

  is_ims = 0;
  dolby_atmos_indicator = 0;
  bool ims_presentation = false;
  bool atmos_presentation = false;
  
  bit_reader.ReadBits(8, &presentation_version);      // preread presentation_version and skip first ReadBits in for loop
  if (presentation_version == 2 && n_presentation > 2) {
    LOG(WARNING) << "Seeing multiple presentations, only single presentation（including IMS presentation) is supported";
    return false;
  }
  if (presentation_version == 1 && n_presentation > 1) {
    LOG(WARNING) << "Seeing multiple presentations, only single presentation (including IMS presentation) is supported";
    return false;
  }
  n_presentation = 1;
  for (uint8_t i = 0; i < n_presentation; i++) {
    if (i != 0) {                                         
      bit_reader.ReadBits(8, &presentation_version);
    }    
    uint32_t pres_bytes;
    bit_reader.ReadBits(8, &pres_bytes);
    if (pres_bytes == 255) {
      uint32_t add_pres_bytes;
      bit_reader.ReadBits(32, &add_pres_bytes);
      pres_bytes += add_pres_bytes;
    }

    uint32_t presentation_bits = 0;
    if (presentation_version == 0) {
      // presentation version 0 is not supported, do nothing
      LOG(WARNING) << "Presentation version 0 is not supported";
      return false;
    } else {
      if (presentation_version == 1 || presentation_version == 2) {
        if (presentation_version == 2) {
          ims_presentation = true;
          is_ims = true;
        }
        // ac4_presentation_v1_dsi()
        const uint32_t presentation_start = bit_reader.bit_position();
        if (!ac4_presentation_v1_dsi(bit_reader, mdcompat, presentation_channel_config, pres_bytes, dolby_atmos_indicator)) {
          return false;
        }
        const uint32_t presentation_end = bit_reader.bit_position();
        presentation_bits = presentation_end - presentation_start;
        if (dolby_atmos_indicator) {
          atmos_presentation = true;
        }
      } else {
        LOG(WARNING) << "Invaild Presentation version";
        return false;
      }
    }
    uint32_t skip_bits = pres_bytes * 8 - presentation_bits;
    bit_reader.SkipBits(skip_bits);
  }
  if (ims_presentation) {
    presentation_version = 2;
  }
  if (atmos_presentation) {
    dolby_atmos_indicator = true;
  }
  return true;
}
}  // namespace

bool CalculateAC4ChannelConfig(const std::vector<uint8_t>& ac4_data,
  uint32_t& channel_config) {
  uint8_t bitstream_version;
  uint8_t presentation_version;
  uint8_t mdcompat;
  uint8_t is_ims;
  uint32_t ch_config = 0;
  uint8_t dolby_atmos_indicator;

  if (!ExtractAc4Data(ac4_data, bitstream_version, presentation_version,
    is_ims, mdcompat, ch_config, dolby_atmos_indicator)) return false;

  if (ch_config == 0) {
    channel_config = 0x800000;
  } else {
    channel_config = ch_config;
  }
  return true;
}

bool CalculateAC4ChannelConfigMpegSchemeValue(const std::vector<uint8_t>& ac4_data,
  uint32_t& mpeg_value) {
  uint8_t bitstream_version;
  uint8_t presentation_version;
  uint8_t mdcompat;
  uint8_t is_ims;
  uint32_t channel_config = 0;
  uint8_t dolby_atmos_indicator;

  if (!ExtractAc4Data(ac4_data, bitstream_version, presentation_version,
    is_ims, mdcompat, channel_config, dolby_atmos_indicator)) return false;

  mpeg_value = MappingChannelConfigtoMpegSchemeValue(channel_config);
  return true;
}

bool GetAc4CodecString(const std::vector<uint8_t>& ac4_data, std::string& codec_string) {
  uint8_t bitstream_version;
  uint8_t presentation_version;
  uint8_t mdcompat;
  uint8_t is_ims;
  uint32_t channel_config;
  uint8_t dolby_atmos_indicator;

  if (!ExtractAc4Data(ac4_data, bitstream_version, presentation_version,
    is_ims, mdcompat, channel_config, dolby_atmos_indicator)) return false;

  codec_string = base::StringPrintf("ac-4.%02d.%02d.%02d",
    bitstream_version, presentation_version, mdcompat);
  return true;
}

bool GetAc4ImsFlag(const std::vector<uint8_t>& ac4_data, uint32_t& payload) {
  uint8_t bitstream_version;
  uint8_t presentation_version;
  uint8_t mdcompat;
  uint8_t is_ims;
  uint32_t channel_config;
  uint8_t dolby_atmos_indicator;

  if (!ExtractAc4Data(ac4_data, bitstream_version, presentation_version,
    is_ims, mdcompat, channel_config, dolby_atmos_indicator)) return false;
  
  payload = (dolby_atmos_indicator << 1) | is_ims;
  return true;
}

}  // namespace media
}  // namespace shaka
