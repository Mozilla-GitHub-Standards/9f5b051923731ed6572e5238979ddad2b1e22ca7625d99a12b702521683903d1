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

uint8_t tv_input_hal_init(void);

uint8_t tv_input_hal_uninit(void);

uint32_t tv_input_hal_get_input_num_by_type(const int type);

uint8_t tv_input_hal_get_inputs_by_type(const uint8_t input_num,
                                        const int type,
                                        int* devices);

uint8_t tv_input_hal_get_stream(uint32_t device_id, tv_stream_t* tv_stream);
