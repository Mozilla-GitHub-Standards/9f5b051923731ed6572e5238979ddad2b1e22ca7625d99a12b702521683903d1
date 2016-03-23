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

#include <assert.h>
#include <fdio/task.h>
#include "service.h"
#include "log.h"
#include "pdu.h"
#include "dtv_io.h"
#include "dtv.h"
#include "tv_hal.h"
#include "dtv_pdu.h"
#include "memptr.h"

enum {
  /* commands/responses */
  OPCODE_ERROR = 0x00,
  OPCODE_GET_TUNERS = 0x01,
  OPCODE_SET_SOURCE = 0x02,
  OPCODE_START_SCAN = 0x03,
  OPCODE_STOP_SCAN = 0x04,
  OPCODE_CLEAR_CACHE = 0x05,
  OPCODE_SET_CHANNEL = 0x06,
  OPCODE_GET_CHANNEL = 0x07,
  OPCODE_GET_PROGRAM = 0x08,
  /* notifications */
  OPCODE_CHANNEL_SCANNED = 0x81,
  OPCODE_SCANNED_COMPLETE = 0x82,
  OPCODE_SCAN_STOPPED = 0x83,
  OPCODE_EIT_BROADCASTED = 0x84
};

static struct dtv_callbacks dtv_callbacks;

static void (*send_pdu)(struct pdu_wbuf* wbuf);

static enum ioresult
send_ntf_pdu(void* data)
{
  /* send notification on I/O thread */
  if (!send_pdu) {
    ALOGE("send_pdu is NULL");
    return IO_OK;
  }
  send_pdu(data);
  return IO_OK;
}

/*
 * Notifications
 */

/*
 * This function is used to notify that new channel is scanned while scanning.
 */
static void
channel_scanned_cb(const char* tuner_id,
                   const uint8_t source_type,
                   const struct tv_channel* ch)
{
  struct pdu_wbuf* wbuf;

  if (!tuner_id) {
    return;
  }

  wbuf = create_pdu_wbuf(strlen(tuner_id) + 1 +  /* Tuner id + '\0'. */
                         sizeof(uint8_t) +       /* Source type. */
                         calculate_ch_size(ch),  /* Channel. */
                         0, NULL);
  if (!wbuf) {
    return;
  }

  init_pdu(&wbuf->buf.pdu, SERVICE_DTV, OPCODE_CHANNEL_SCANNED);
  if (append_to_pdu(&wbuf->buf.pdu, "0C", tuner_id, source_type) < 0) {
    goto cleanup;
  }

  if (append_channel(&wbuf->buf.pdu, ch) < 0) {
    goto cleanup;
  }

  if (run_task(send_ntf_pdu, wbuf) < 0) {
    goto cleanup;
  }

  return;
cleanup:
  destroy_pdu_wbuf(wbuf);
}

/*
 * This function is used to notify that scanning process is complete.
 */
static void
scanned_complete_cb(const char* tuner_id,
                   const uint8_t source_type)
{
  struct pdu_wbuf* wbuf;

  if (!tuner_id) {
    return;
  }

  wbuf = create_pdu_wbuf(strlen(tuner_id) + 1 + /* Tuner id + '\0'. */
                         sizeof(uint8_t),       /* Source type. */
                         0, NULL);
  if (!wbuf) {
    return;
  }

  init_pdu(&wbuf->buf.pdu, SERVICE_DTV, OPCODE_SCANNED_COMPLETE);
  if (append_to_pdu(&wbuf->buf.pdu, "0C", tuner_id, source_type) < 0) {
    goto cleanup;
  }

  if (run_task(send_ntf_pdu, wbuf) < 0) {
    goto cleanup;
  }

  return;
cleanup:
  destroy_pdu_wbuf(wbuf);
}

/*
 * This function is used to notify that new channel is stopped.
 */
static void
scanned_stopped_cb (const char* tuner_id,
                    const uint8_t source_type)
{
  struct pdu_wbuf* wbuf;

  if (!tuner_id) {
    return;
  }

  wbuf = create_pdu_wbuf(strlen(tuner_id) + 1 + /* Tuner id + '0'. */
                         sizeof(uint8_t),       /* Source type. */
                         0, NULL);
  if (!wbuf) {
    return;
  }

  init_pdu(&wbuf->buf.pdu, SERVICE_DTV, OPCODE_SCAN_STOPPED);
  if (append_to_pdu(&wbuf->buf.pdu, "0C", tuner_id, source_type) < 0) {
    goto cleanup;
  }

  if (run_task(send_ntf_pdu, wbuf) < 0) {
    goto cleanup;
  }

  return;
cleanup:
  destroy_pdu_wbuf(wbuf);
}

/*
 * This function is used to notify that a event input table is received.
 */
static void
eit_broadcasted_cb(const char* tuner_id,
                   const uint8_t source_type,
                   const struct tv_channel* ch,
                   const uint32_t prog_num,
                   const struct tv_program* progs)
{
  struct pdu_wbuf* wbuf;
  uint32_t pdu_size;
  uint32_t prog_size;
  uint32_t prog_idx;

  if (!tuner_id) {
    return;
  }

  prog_size = 0;
  for (prog_idx = 0; prog_idx < prog_num; prog_idx++) {
    prog_size += calculate_prog_size(&progs[prog_idx]);
  }

  wbuf = create_pdu_wbuf(strlen(tuner_id) + 1 +   /* Tuner id + '0'. */
                         sizeof(uint8_t) +        /* Source type. */
                         calculate_ch_size(ch) +  /* Channel. */
                         sizeof(uint32_t) +       /* Number of programs. */
                         prog_size,               /* Programs. */
                         0, NULL);
  if (!wbuf) {
    return;
  }

  init_pdu(&wbuf->buf.pdu, SERVICE_DTV, OPCODE_EIT_BROADCASTED);

  if (append_to_pdu(&wbuf->buf.pdu, "0C", tuner_id, source_type) < 0) {
    goto cleanup;
  }

  if (append_channel(&wbuf->buf.pdu, ch) < 0) {
    goto cleanup;
  }

  if (append_to_pdu(&wbuf->buf.pdu, "I", prog_num) < 0) {
    goto cleanup;
  }

  for (prog_idx = 0; prog_idx < prog_num; prog_idx++){
    if (append_program(&wbuf->buf.pdu, &progs[prog_idx]) < 0 ) {
      goto cleanup;
    }
  }

  if (run_task(send_ntf_pdu, wbuf) < 0) {
    goto cleanup;
  }

  return;

cleanup:
  destroy_pdu_wbuf(wbuf);
}

static void
channel_update_nfy_cb(uint8_t ch_status,
                      const char* tuner_id,
                      const uint8_t source_type,
                      const struct tv_channel* ch)
{
  if (ch_status == DTV_CHANNEL_ADD) {
    channel_scanned_cb(tuner_id, source_type, ch);
  } else {
    ALOGW("Unknown status %d", ch_status);
  }
}

static void
scan_status_nfy_cb(uint8_t scan_status,
                   const char* tuner_id,
                   const uint8_t source_type)
{
  if (scan_status == DTV_SCAN_COMPLETE) {
    scanned_complete_cb(tuner_id, source_type);
  } else if (scan_status == DTV_SCAN_STOPPED) {
    scanned_stopped_cb(tuner_id, source_type);
  } else {
    ALOGW("Unknown status %d", scan_status);
  }
}

static void
event_nfy_cb(const char* tuner_id,
             const uint8_t source_type,
             const struct tv_channel* ch,
             const uint32_t prog_num,
             const struct tv_program* progs)
{
  eit_broadcasted_cb(tuner_id, source_type, ch, prog_num, progs);
}

/*
 * Commands/Responses
 */

/*
 * This function pass the input sources which type is TV tuner back.
 */
static int
get_tuners(const struct pdu* cmd)
{
  struct pdu_wbuf* wbuf;
  uint32_t tuner_num;
  struct tv_tuner* tuners;

  tuner_num = dtv_get_tuner_num();
  tuners = (struct tv_tuner*)malloc(sizeof(struct tv_tuner) * tuner_num);

  if (dtv_get_tuners(tuner_num, tuners) != TV_STATUS_SUCCESS) {
    return ERROR_FAIL;
  }

  /* Calculate pdu size. */
  uint32_t pdu_size;
  uint32_t tuner_idx;

  pdu_size = sizeof(uint32_t); /* Numbers of tuners. */

  /* Calculate size of tuners. */
  for (tuner_idx = 0; tuner_idx < tuner_num; tuner_idx++) {
    pdu_size += calculate_tuner_size(&tuners[tuner_idx]);
  }

  wbuf = create_pdu_wbuf(pdu_size, 0, NULL);
  if (!wbuf) {
    return ERROR_NOMEM;
  }

  init_pdu(&wbuf->buf.pdu, cmd->service, cmd->opcode);

  if (append_to_pdu(&wbuf->buf.pdu, "I", tuner_num) < 0) {
    goto cleanup;
  }

  for (tuner_idx = 0; tuner_idx < tuner_num; tuner_idx++){
    if (append_tuner(&wbuf->buf.pdu, &tuners[tuner_idx]) < 0) {
      goto cleanup;
    }
  }

  send_pdu(wbuf);
  release_tuners(tuner_num, tuners);

  return ERROR_NONE;

cleanup:
  release_tuners(tuner_num, tuners);
  destroy_pdu_wbuf(wbuf);
  return ERROR_NOMEM;
}

/*
 * This function set the signal source and pass the TV stream back.
 */
static int
set_source(const struct pdu* cmd)
{
  struct pdu_wbuf* wbuf;
  char* tuner_id;
  uint8_t source_type;
  tv_stream_t tv_stream;
  native_handle_t* native_handle;
  struct ancillary_data* tail_data;
  uint8_t ret;
  int32_t idx;
  int fd_num;

  if (read_pdu_at(cmd, 0, "0C", &tuner_id, &source_type) < 0) {
    return ERROR_FAIL;
  }

  ret = dtv_set_source(tuner_id, source_type, &tv_stream);
  if (ret != TV_STATUS_SUCCESS) {
    return ret;
  }

  native_handle = tv_stream.sideband_stream_source_handle;

  if (native_handle->numFds == 0) {
    wbuf = create_pdu_wbuf(sizeof(uint32_t) +                    /* version */
                           sizeof(uint32_t) +                    /* numFds */
                           sizeof(uint32_t) +                    /* numInts */
                           sizeof(int) * native_handle->numInts, /* data */
                           0,
                           NULL);

  } else {
    fd_num = native_handle->numFds;
    wbuf = create_pdu_wbuf(sizeof(uint32_t) +                    /* version */
                           sizeof(uint32_t) +                    /* numFds */
                           sizeof(uint32_t) +                    /* numInts */
                           sizeof(int) * native_handle->numInts, /* data */
                           ALIGNMENT_PADDING +
                           sizeof(*tail_data) + (sizeof(int) * fd_num) +
                           (sizeof(unsigned char) *              /* Space for */
                            CMSG_SPACE(sizeof(int) * fd_num)),   /* control   */
                           build_ancillary_data);                /* message.  */

    tail_data = ceil_align(pdu_wbuf_tail(wbuf), ALIGNMENT_PADDING);
    tail_data->fd_num = fd_num;
    for (idx = 0; idx < fd_num; idx++) {
      tail_data->data[idx] = native_handle->data[idx];
    }
  }

  if (!wbuf) {
    return ERROR_NOMEM;
  }

  init_pdu(&wbuf->buf.pdu, cmd->service, cmd->opcode);

  if (append_to_pdu(&wbuf->buf.pdu, "III", native_handle->version
                                         , native_handle->numFds
                                         , native_handle->numInts) < 0) {
    goto cleanup;
  }

  int32_t data_idx;

  for (data_idx = native_handle->numFds;
       data_idx < native_handle->numInts; data_idx++) {
    if (append_to_pdu(&wbuf->buf.pdu, "I", native_handle->data[data_idx]) < 0) {
      goto cleanup;
    }
  }

  send_pdu(wbuf);

  return ERROR_NONE;

cleanup:
  destroy_pdu_wbuf(wbuf);
  return ERROR_NOMEM;
}

static int
start_scan(const struct pdu* cmd)
{
  struct pdu_wbuf* wbuf;
  char* tuner_id;
  uint8_t source_type;
  uint8_t ret;

  if (read_pdu_at(cmd, 0, "0C", &tuner_id, &source_type) < 0) {
    return ERROR_FAIL;
  }

  ret = dtv_start_scanning(tuner_id, source_type);
  if (ret != TV_STATUS_SUCCESS) {
    return ret;
  }

  wbuf = create_pdu_wbuf(0, 0, NULL);
  if (!wbuf) {
    return ERROR_NOMEM;
  }

  init_pdu(&wbuf->buf.pdu, cmd->service, cmd->opcode);
  send_pdu(wbuf);

  return ERROR_NONE;
}

static int
stop_scan(const struct pdu* cmd)
{
  struct pdu_wbuf* wbuf;
  char* tuner_id;
  uint8_t source_type;
  uint8_t ret;

  if (read_pdu_at(cmd, 0, "0C", &tuner_id, &source_type) < 0) {
    return ERROR_FAIL;
  }

  ret = dtv_stop_scanning(tuner_id, source_type);
  if (ret != TV_STATUS_SUCCESS) {
    return ret;
  }

  wbuf = create_pdu_wbuf(0, 0, NULL);
  if (!wbuf) {
    return ERROR_NOMEM;
  }

  init_pdu(&wbuf->buf.pdu, cmd->service, cmd->opcode);
  send_pdu(wbuf);

  return ERROR_NONE;
}

static int
clean_cache(const struct pdu* cmd)
{
  struct pdu_wbuf* wbuf;
  uint8_t ret;

  ret = dtv_cln_scanned_channel_cache();
  if (ret != TV_STATUS_SUCCESS) {
    return ret;
  }

  wbuf = create_pdu_wbuf(0, 0, NULL);
  if (!wbuf) {
    return ERROR_NOMEM;
  }

  init_pdu(&wbuf->buf.pdu, cmd->service, cmd->opcode);
  send_pdu(wbuf);

  return ERROR_NONE;
}

static int
set_channel(const struct pdu* cmd)
{
  struct pdu_wbuf* wbuf;
  char* tuner_id;
  uint8_t source_type;
  char* ch_num;
  struct tv_channel* ch;
  uint32_t ch_size;
  uint8_t ret;

  if (read_pdu_at(cmd, 0, "0C0", &tuner_id, &source_type, &ch_num) < 0) {
    return ERROR_FAIL;
  }

  ch = (struct tv_channel*)malloc(sizeof(struct tv_channel));

  ret = dtv_set_channel(tuner_id, source_type, ch_num, ch);
  if (ret != TV_STATUS_SUCCESS) {
    release_channels(1, ch);
    return ret;
  }

  ch_size = calculate_ch_size(ch);
  wbuf = create_pdu_wbuf(ch_size, 0, NULL);
  if (!wbuf) {
    return ERROR_NOMEM;
  }

  init_pdu(&wbuf->buf.pdu, cmd->service, cmd->opcode);

  if (append_channel(&wbuf->buf.pdu, ch) < 0) {
    goto cleanup;
  }

  send_pdu(wbuf);
  release_channels(1, ch);

  return ERROR_NONE;

cleanup:
  release_channels(1, ch);
  destroy_pdu_wbuf(wbuf);
  return ERROR_NOMEM;
}

static int
get_channels(const struct pdu* cmd)
{
  struct pdu_wbuf* wbuf;
  uint32_t ch_num;
  struct tv_channel* ch_list;
  char* tuner_id;
  uint8_t source_type;
  uint32_t ch_list_size;
  uint32_t ch_idx;
  uint8_t ret;

  if (read_pdu_at(cmd, 0, "0C", &tuner_id, &source_type) < 0) {
    return ERROR_FAIL;
  }

  ch_num = dtv_get_channel_num(tuner_id, source_type);
  ch_list = (struct tv_channel*)malloc(sizeof(struct tv_channel) * ch_num);

  ret = dtv_get_channels(tuner_id, source_type, ch_num, ch_list);
  if (ret != TV_STATUS_SUCCESS) {
    return ret;
  }

  ch_list_size = sizeof(uint32_t); /* Number of channels. */
  for (ch_idx = 0; ch_idx < ch_num; ch_idx++) {
    /* Calculate size of channel. */
    ch_list_size += calculate_ch_size(&ch_list[ch_idx]);
  }

  wbuf = create_pdu_wbuf(ch_list_size, 0, NULL);
  if (!wbuf) {
    return ERROR_NOMEM;
  }

  init_pdu(&wbuf->buf.pdu, cmd->service, cmd->opcode);

  if (append_to_pdu(&wbuf->buf.pdu, "I", ch_num) < 0) {
    goto cleanup;
  }

  for (ch_idx = 0; ch_idx < ch_num; ch_idx++) {
    if (append_channel(&wbuf->buf.pdu, &ch_list[ch_idx]) < 0) {
      goto cleanup;
    }
  }

  send_pdu(wbuf);
  release_channels(ch_num, ch_list);

  return ERROR_NONE;

cleanup:
  release_channels(ch_num, ch_list);
  destroy_pdu_wbuf(wbuf);
  return ERROR_NOMEM;
}

static int
get_programs(const struct pdu* cmd)
{
  struct pdu_wbuf* wbuf;
  char* tuner_id;
  uint8_t source_type;
  char* ch_num;
  struct tv_channel* ch;
  uint64_t start_time;
  uint64_t end_time;
  struct tv_program* prog_list;
  uint32_t prog_num;
  uint32_t pdu_size;
  uint32_t prog_idx;
  uint8_t ret;

  if (read_pdu_at(cmd, 0, "0C0LL", &tuner_id, &source_type, &ch_num,
                                   &start_time, &end_time) < 0) {
    return ERROR_FAIL;
  }

  prog_num = dtv_get_prog_num(tuner_id, source_type, ch_num,
                              start_time, end_time);
  prog_list = (struct tv_program*)malloc(sizeof(struct tv_program) * prog_num);

  ret = dtv_get_programs(tuner_id, source_type, ch_num, start_time, end_time,
                         prog_num, prog_list);
  if (ret != TV_STATUS_SUCCESS) {
    return ret;
  }

  pdu_size = sizeof(uint32_t); /* Number of programs. */
  for (prog_idx = 0; prog_idx < prog_num; prog_idx++) {
    /* Calculate size of program. */
    pdu_size += calculate_prog_size(&prog_list[prog_idx]);
  }

  wbuf = create_pdu_wbuf(pdu_size, 0, NULL);
  if (!wbuf) {
    return ERROR_NOMEM;
  }

  init_pdu(&wbuf->buf.pdu, cmd->service, cmd->opcode);

  if (append_to_pdu(&wbuf->buf.pdu, "I", prog_num) < 0) {
    goto cleanup;
  }

  for (prog_idx = 0; prog_idx < prog_num; prog_idx++) {
    if (append_program(&wbuf->buf.pdu, &prog_list[prog_idx]) < 0 ) {
      goto cleanup;
    }
  }

  send_pdu(wbuf);
  release_programs(prog_num, prog_list);

  return ERROR_NONE;

cleanup:
  release_programs(prog_num, prog_list);
  destroy_pdu_wbuf(wbuf);
  return ERROR_NOMEM;
}

static int
dtv_handler(const struct pdu* cmd)
{
  static int (* const handler[256])(const struct pdu*) = {
    [OPCODE_GET_TUNERS] = get_tuners,
    [OPCODE_SET_SOURCE] = set_source,
    [OPCODE_START_SCAN] = start_scan,
    [OPCODE_STOP_SCAN] = stop_scan,
    [OPCODE_CLEAR_CACHE] = clean_cache,
    [OPCODE_SET_CHANNEL] = set_channel,
    [OPCODE_GET_CHANNEL] = get_channels,
    [OPCODE_GET_PROGRAM] = get_programs,
  };

  return handle_pdu_by_opcode(cmd, handler);
}

int
(*register_dtv(void (*send_pdu_cb)(struct pdu_wbuf*)))(const struct pdu*)
{
  int32_t ret;

  ALOGD("Start init dtv.");

  /* Init Android TV HAL. */
  ret = tv_input_hal_init();
  if (ret != TV_STATUS_SUCCESS) {
    tv_input_hal_uninit();
    return NULL;
  }

  dtv_callbacks.channel_update_nfy_cb = &channel_update_nfy_cb;
  dtv_callbacks.scan_status_nfy_cb = &scan_status_nfy_cb;
  dtv_callbacks.event_nfy_cb = &event_nfy_cb;
  /* Init vendor DTV API. */
  ret = dtv_init(&dtv_callbacks);
  if (ret != TV_STATUS_SUCCESS) {
    dtv_uninit();
    return NULL;
  }

  send_pdu = send_pdu_cb;

  return dtv_handler;
}

int
unregister_dtv(void)
{
  int32_t ret;

  /* Init Android TV HAL. */
  ret = tv_input_hal_uninit();
  if (ret != TV_STATUS_SUCCESS) {
    return ERROR_FAIL;
  }

  /* Init vendor DTV API. */
  ret = dtv_uninit();
  if (ret != TV_STATUS_SUCCESS) {
    return ERROR_FAIL;
  }

  send_pdu = NULL;

  return ERROR_NONE;
}
