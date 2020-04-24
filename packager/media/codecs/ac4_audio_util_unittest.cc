// Copyright 2016 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>

#include "packager/media/codecs/ac4_audio_util.h"

namespace shaka {
namespace media {

TEST(AC4AudioUtilTest, ChannelTest1) {
  // audio_coding_mode is 7, which is Left, Center, Right, Left surround, Right
  // surround. No dependent substreams. LFE channel on.
  const std::vector<uint8_t> ac4_data = {0x20, 0xa6, 0x02, 0x40, 0x00, 0x00, 
                                         0x00, 0x1f, 0xff, 0xff, 0xff, 0xe0, 
                                         0x02, 0x12, 0xf8, 0x80, 0x00, 0x00, 
                                         0x42, 0x00, 0x00, 0x02, 0x50, 0x10};

  uint32_t channel_config;
  uint32_t channel_config_mpeg_value;
  std::string codec;
  uint32_t is_ims;

  EXPECT_TRUE(CalculateAC4ChannelConfig(ac4_data, channel_config));
  EXPECT_EQ((uint32_t)0x1, channel_config);
  EXPECT_TRUE(CalculateAC4ChannelConfigMpegValue(ac4_data,
                                                 channel_config_mpeg_value));
  EXPECT_EQ((uint32_t)0x2, channel_config_mpeg_value);
  EXPECT_TRUE(GetAc4CodecString(ac4_data, codec));
  EXPECT_EQ((std::string)"ac-4.02.02.00", codec);
  EXPECT_TRUE(GetAc4ImsFlag(ac4_data, is_ims));
  EXPECT_EQ((uint32_t)0x1, is_ims);
}


}  // namespace media
}  // namespace shaka
