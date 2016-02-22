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

#include "tv_utils.h"

void
release_tuners(const uint32_t num, struct tv_tuner* tuners)
{
  uint32_t idx;
  if (!tuners) {
    return;
  }
  for (idx = 0; idx < num; idx++) {
    free(tuners[idx].id);
    free(tuners[idx].supported_types);
  }
  free(tuners);
}

void
release_channels(const uint32_t num, struct tv_channel* channels)
{
  uint32_t idx;

  if (!channels) {
    return;
  }
  for (idx = 0; idx < num; idx++) {
    free(channels[idx].network_id);
    free(channels[idx].trans_stream_id);
    free(channels[idx].service_id);
    free(channels[idx].number);
    free(channels[idx].name);
  }
  free(channels);
}

void
release_programs(const uint32_t num, struct tv_program* programs)
{
  uint32_t prog_idx;
  uint32_t attr_idx;

  if (!programs) {
    return;
  }

  for (prog_idx = 0; prog_idx < num; prog_idx++) {
    free(programs[prog_idx].evt_id);
    free(programs[prog_idx].title);
    free(programs[prog_idx].descpt);
    free(programs[prog_idx].rating);

    for (attr_idx=0; attr_idx<programs[prog_idx].lang_num; attr_idx++) {
      free(programs[prog_idx].langs[attr_idx]);
    }
    free(programs[prog_idx].langs);

    for (attr_idx=0; attr_idx<programs[prog_idx].stl_lang_num; attr_idx++) {
      free(programs[prog_idx].stl_langs[attr_idx]);
    }
    free(programs[prog_idx].stl_langs);

    free(programs);
  }
}
