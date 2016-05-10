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

/*
 * This file contains the implementation of the DTV service for Android
 * tv_inputs and provide an interface TV-related functionalitied implementation.
 * More detailed document is at [1].
 * [1] https://source.android.com/devices/halref/tv__input_8h_source.html
 */

#include <string.h>
#include "dtv.h"
#include "dtv_io.h"
#include "tv_hal.h"

/*
 * This method is used to calculate the number character for a string to show
 * the target number.
 */
static uint8_t
get_int_length(uint32_t target)
{
  uint8_t len = 1;
  while (target > 10) {
    target /= 10;
    len++;
  }
  return len;
}

uint8_t
dtv_init(struct dtv_callbacks* dtv_callbacks)
{
  return TV_STATUS_SUCCESS;
}

uint8_t
dtv_uninit()
{
  return TV_STATUS_SUCCESS;
}

uint32_t
dtv_get_tuner_num()
{
  return tv_input_hal_get_input_num_by_type(TV_INPUT_TYPE_TUNER);
}

uint8_t
dtv_get_tuners(const uint32_t tuner_num,
               struct tv_tuner* tuners)
{
  uint32_t idx;
  int* tuner_id_list;
  uint8_t ret;

  tuner_id_list = (int*)malloc(sizeof(int) * tuner_num);

  ret = tv_input_hal_get_inputs_by_type(tuner_num, TV_INPUT_TYPE_TUNER,
                                        tuner_id_list);
  if (ret != TV_STATUS_SUCCESS) {
    free(tuner_id_list);
    return TV_STATUS_FAIL;
  }

  for (idx = 0; idx < tuner_num; idx++) {
    uint8_t len = 0;
    len = get_int_length((uint32_t)tuner_id_list[idx]);
    tuners[idx].id = (char*)malloc(sizeof(char) * (len + 1));
    snprintf(tuners[idx].id, len + 1, "%d", tuner_id_list[idx]);
    tuners[idx].num_types = 0;
  }
  free(tuner_id_list);

  return TV_STATUS_SUCCESS;
}

uint8_t
dtv_set_source(const char* tuner_id, const uint8_t source_type,
               tv_stream_t* tv_stream)
{
  uint8_t ret;
  uint32_t idx;
  uint32_t device_id;
  uint32_t length;

  device_id = 0;
  length = strlen(tuner_id);

  for (idx = 0; idx < length; idx++) {
    device_id = (device_id*10) + (tuner_id[idx] - '0');
  }

  return tv_input_hal_get_stream(device_id, tv_stream);
}

uint8_t
dtv_start_scanning(const char* tuner_id, const uint8_t source_type)
{
  return TV_STATUS_NOT_SUPPORTED;
}

uint8_t
dtv_stop_scanning(const char* tuner_id, const uint8_t source_type)
{
  return TV_STATUS_NOT_SUPPORTED;
}

uint8_t
dtv_cln_scanned_channel_cache()
{
  return TV_STATUS_NOT_SUPPORTED;
}

uint8_t
dtv_set_channel(const char* tuner_id,
                const uint8_t source_type,
                const char* channel_num,
                struct tv_channel* ch)
{
  return TV_STATUS_NOT_SUPPORTED;
}

uint32_t
dtv_get_channel_num(const char* tuner_id,
                    const uint8_t source_type)
{
  return 0;
}

uint8_t
dtv_get_channels(const char* tuner_id,
                 const uint8_t source_type,
                 const uint32_t ch_num,
                 struct tv_channel* ch)
{
  return TV_STATUS_NOT_SUPPORTED;
}

uint32_t
dtv_get_prog_num(const char* tuner_id,
                 const uint8_t source_type,
                 const char* ch_num,
                 const uint64_t start_time,
                 const uint64_t end_time)
{
  return 0;
}

uint8_t
dtv_get_programs(const char* tuner_id,
                 const uint8_t source_type,
                 const char* ch_num,
                 const uint64_t start_time,
                 const uint64_t end_time,
                 const uint32_t prog_num,
                 struct tv_program* progs)
{
  return TV_STATUS_NOT_SUPPORTED;
}
