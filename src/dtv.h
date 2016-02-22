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
#include "tv_utils.h"

struct dtv_callbacks{
  void
  (*channel_update_nfy_cb)(uint8_t scan_status,
                           const char* tuner_id,
                           const uint8_t source_type,
                           const struct tv_channel* ch);
  void
  (*scan_status_nfy_cb)(uint8_t scan_status,
                        const char* tuner_id,
                        const uint8_t source_type);
  void
  (*event_nfy_cb)(const char* tuner_id,
                  const uint8_t source_type,
                  const struct tv_channel* ch,
                  const uint32_t prog_num,
                  const struct tv_program* progs);
};


uint8_t dtv_init(struct dtv_callbacks* dtv_callback);

uint8_t dtv_uninit(void);

uint32_t dtv_get_tuner_num();

uint8_t dtv_get_tuners(const uint32_t tuner_num, struct tv_tuner* tuners);

uint8_t dtv_set_source(const char* tuner_id, const uint8_t source_type,
                       tv_stream_t* stream);

uint8_t dtv_start_scanning(const char* tuner_id, const uint8_t source_type);

uint8_t dtv_stop_scanning(const char* tuner_id, const uint8_t source_type);

uint8_t dtv_cln_scanned_channel_cache();

uint8_t dtv_set_channel(const char* tuner_id,
                        const uint8_t source_type,
                        const char* channel_num,
                        struct tv_channel* ch);

uint32_t dtv_get_channel_num(const char* tuner_id,
                             const uint8_t source_type);

uint8_t dtv_get_channels(const char* tuner_id,
                         const uint8_t source_type,
                         const uint32_t ch_num,
                         struct tv_channel* ch);

uint32_t dtv_get_prog_num(const char* tuner_id,
                          const uint8_t source_type,
                          const char* ch_num,
                          const uint64_t start_time,
                          const uint64_t end_time);

uint8_t dtv_get_programs(const char* tuner_id,
                         const uint8_t source_type,
                         const char* ch_num,
                         const uint64_t start_time,
                         const uint64_t end_time,
                         const uint32_t prog_num,
                         struct tv_program* progs);
