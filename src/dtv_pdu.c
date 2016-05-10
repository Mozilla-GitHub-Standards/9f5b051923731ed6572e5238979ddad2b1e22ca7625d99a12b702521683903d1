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

#include <pdu/pdubuf.h>
#include "dtv_pdu.h"
#include "memptr.h"
#include "assert.h"

int
build_ancillary_data(struct pdu_wbuf* wbuf, struct msghdr* msg)
{
  struct ancillary_data* tail_data;
  struct cmsghdr* chdr;

  assert(msg);

  tail_data = ceil_align(pdu_wbuf_tail(wbuf), ALIGNMENT_PADDING);

  /* Buffer for ancillary data is append after tail_data. */
  msg->msg_control = &tail_data->data[tail_data->fd_num];

  /* Assign the maximum length. */
  msg->msg_controllen = CMSG_SPACE(sizeof(int) * tail_data->fd_num);

  chdr = CMSG_FIRSTHDR(msg);
  chdr->cmsg_level = SOL_SOCKET;
  chdr->cmsg_type = SCM_RIGHTS;
  chdr->cmsg_len = CMSG_LEN(sizeof(int) * tail_data->fd_num);
  memcpy(CMSG_DATA(chdr), &tail_data->data, sizeof(int) * tail_data->fd_num);
  msg->msg_controllen = chdr->cmsg_len; /* Assign actual ancillary length. */

  return 0;
}

uint32_t
calculate_tuner_size(const struct tv_tuner* tuner)
{
  uint32_t size = 0;

  if (!tuner) {
    return 0;
  }

  size += sizeof(uint32_t); /* num_types */
  size += strlen(tuner->id) + 1;
  size += tuner->num_types;

  return size;
}

uint32_t
calculate_ch_size(const struct tv_channel* ch)
{
  uint32_t size = 0;

  if (!ch) {
    return 0;
  }

  size += strlen(ch->network_id) + 1;
  size += strlen(ch->trans_stream_id) + 1;
  size += strlen(ch->service_id) + 1;
  size += sizeof(uint8_t); /* type */
  size += strlen(ch->number) + 1;
  size += strlen(ch->name) + 1;
  size += sizeof(uint8_t); /* is_emergency */
  size += sizeof(uint8_t); /* is_free */

  return size;
}

uint32_t
calculate_prog_size(const struct tv_program* prog)
{
  uint32_t idx;
  uint32_t size = 0;

  if (!prog) {
    return 0;
  }

  size += strlen(prog->evt_id) + 1;
  size += strlen(prog->title) + 1;
  size += sizeof(uint64_t); /* start_time */
  size += sizeof(uint64_t); /* end_time */
  size += strlen(prog->descpt) + 1;
  size += strlen(prog->rating) + 1;
  size += sizeof(uint32_t); /* lang_num */
  for (idx = 0; idx < prog->lang_num; idx++) {
    size += strlen(prog->langs[idx]) + 1;
  }
  size += sizeof(uint32_t); /* stl_lang_num */
  for (idx = 0; idx < prog->lang_num; idx++) {
    size += strlen(prog->stl_langs[idx]) + 1;
  }

  return size;
}

long
append_tuner(struct pdu* pdu, const struct tv_tuner* tuner)
{
  long res;

  if (append_to_pdu(pdu, "0I", tuner->id, tuner->num_types) < 0) {
    return -1;
  }

  uint32_t source_type_idx;
  for (source_type_idx = 0; source_type_idx < tuner->num_types; source_type_idx++) {
    uint8_t sup_type;
    sup_type = tuner->supported_types[source_type_idx];
    if(append_to_pdu(pdu, "C", sup_type) < 0) {
      return -1;
    }
  }

  return TV_STATUS_SUCCESS;
}

long
append_channel(struct pdu* pdu, const struct tv_channel* ch)
{
  if (append_to_pdu(pdu, "000C00CC", ch->network_id, ch->trans_stream_id
                                   , ch->service_id, ch->type, ch->number
                                   , ch->is_emergency, ch->is_free) < 0) {
    return -1;
  }

  return TV_STATUS_SUCCESS;
}

long
append_program(struct pdu* pdu, const struct tv_program* prog)
{
  uint32_t idx;

  if (append_to_pdu(pdu, "0LL00I", prog->evt_id, prog->start_time
                                 , prog->duration, prog->descpt
                                 , prog->rating, prog->lang_num) < 0) {
    return -1;
  }

  for (idx = 0; idx < prog->lang_num; idx++) {
    if (append_to_pdu(pdu, "0", prog->langs[idx]) < 0) {
      return -1;
    }
  }

  if (append_to_pdu(pdu, "I", prog->stl_lang_num) < 0) {
    return -1;
  }

  for (idx = 0; idx < prog->lang_num; idx++) {
    if (append_to_pdu(pdu, "0", prog->stl_langs[idx]) < 0) {
      return -1;
    }
  }

  return TV_STATUS_SUCCESS;
}
