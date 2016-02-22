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

#include <sys/socket.h>
#include <pdu/pdubuf.h>
#include "tv_utils.h"
#include "pdu.h"

uint32_t calculate_tuner_size(const struct tv_tuner* tuner);

uint32_t calculate_ch_size(const struct tv_channel* ch);

uint32_t calculate_prog_size(const struct tv_program* prog);

long append_tuner(struct pdu* pdu, const struct tv_tuner* tuner);

long append_channel(struct pdu* pdu, const struct tv_channel* ch);

long append_program(struct pdu* pdu, const struct tv_program* prog);
