/*
 * Copyright (C) 2015-2016  Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <hardware/tv_input.h>

#define TV_STATUS_SUCCESS       0
#define TV_STATUS_FAIL          1
#define TV_STATUS_INVARG        2
#define TV_STATUS_NO_SIGNAL     3
#define TV_STATUS_NOT_SUPPORTED 4

/* Channel status */
#define DTV_CHANNEL_ADD 0

/* Scan status */
#define DTV_SCAN_UNKNOWN  0
#define DTV_SCAN_COMPLETE 1
#define DTV_SCAN_STOPPED  2

typedef enum {
  TVD_DVB_T    = 0x00,
  TVD_DVB_T2   = 0x01,
  TVD_DVB_C    = 0x02,
  TVD_DVB_C2   = 0x03,
  TVD_DVB_S    = 0x04,
  TVD_DVB_S2   = 0x05,
  TVD_DVB_H    = 0x06,
  TVD_DVB_SH   = 0x07,
  TVD_ATSC     = 0x08,
  TVD_ATSC_M_H = 0x09,
  TVD_ISDB_T   = 0x0a,
  TVD_ISDB_Tb  = 0x0b,
  TVD_ISDB_S   = 0x0c,
  TVD_ISDB_C   = 0x0d,
  TVD_ONE_SEG  = 0x0e,
  TVD_DTMB     = 0x0f,
  TVD_CMMB     = 0x10,
  TVD_T_DMB    = 0x11,
  TVD_S_DMB    = 0x12
} tvd_tuner_type;

struct tv_tuner {
  char* id;
  uint32_t num_types;
  uint8_t* supported_types;
};

struct tv_channel {
  char* network_id;
  char* trans_stream_id;
  char* service_id;
  char type;
  char* number;
  char* name;
  char is_emergency;
  char is_free;
};

struct tv_program {
  char* evt_id;
  char* title;
  uint64_t start_time;
  uint64_t duration;
  char* descpt;
  char* rating;
  uint32_t lang_num;
  char** langs;
  uint32_t stl_lang_num;
  char** stl_langs;
};

void release_tuners(const uint32_t num, struct tv_tuner* tuners);

void release_channels(const uint32_t num, struct tv_channel* channels);

void release_programs(const uint32_t num, struct tv_program* programs);
