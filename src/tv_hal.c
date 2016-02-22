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

#include <hardware/tv_input.h>
#include "tv_utils.h"
#include "log.h"

#define MAX_DEVICE_NUM 100

struct device_info {
  int device_id;
  int type;
  int acting_stream_id;
};

static tv_input_module_t* module = NULL;
static tv_input_device_t* device = NULL;
static tv_input_callback_ops_t tv_callback;
static uint8_t device_num;
static struct device_info device_list[MAX_DEVICE_NUM];

static void
remove_device_by_idx(uint8_t target)
{
  uint8_t idx;
  device_num--;
  for (idx = target; idx < device_num; idx++) {
    device_list[idx].device_id = device_list[idx + 1].device_id;
    device_list[idx].type = device_list[idx + 1].type;
    device_list[idx].acting_stream_id = device_list[idx + 1].acting_stream_id;
  }
}

static void
device_init_notify(struct tv_input_device* dev,
        tv_input_event_t* event, void* data)
{
  uint8_t idx;

  /* We only handle tuner device in current phase. */
  if (event->type == TV_INPUT_EVENT_DEVICE_AVAILABLE ||
      event->type == TV_INPUT_EVENT_DEVICE_UNAVAILABLE ||
      event->type == TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED) {
    ALOGD("Notify %d %d", event->device_info.device_id, event->device_info.type);
    if (event->device_info.type != TV_INPUT_TYPE_TUNER) {
      ALOGD("Not a tuner hardware");
      return;
    }
  }

  switch(event->type) {
    case TV_INPUT_EVENT_DEVICE_AVAILABLE :
      ALOGD("TV_INPUT_EVENT_DEVICE_AVAILABLE %d", event->device_info.device_id);
      if (device_num >= MAX_DEVICE_NUM) {
        ALOGE("Device list overflow.");
        return;
      }
      device_list[device_num].device_id = event->device_info.device_id;
      device_list[device_num].type = event->device_info.type;
      device_list[device_num].acting_stream_id = -1;
      device_num++;
      break;
    case TV_INPUT_EVENT_DEVICE_UNAVAILABLE :
      ALOGD("TV_INPUT_EVENT_DEVICE_UNAVAILABLE %d", event->device_info.device_id);
      for (idx = 0; idx < device_num; idx++) {
        if (device_list[idx].device_id == event->device_info.device_id) {
          break;
        }
      }

      if (idx >= device_num) {
        ALOGE("Unknown device ID.");
        return;
      }

      device->close_stream(device,
                           event->device_info.device_id,
                           device_list[idx].acting_stream_id);
      remove_device_by_idx(idx);
      break;
    case TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED :
      ALOGD("TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED %d", event->device_info.device_id);
      for (idx = 0; idx < device_num; idx++) {
        if (device_list[idx].device_id == event->device_info.device_id) {
          break;
        }
      }

      if (idx >= device_num) {
        ALOGE("Unknown device ID.");
        return;
      }

      device->close_stream(device,
                           event->device_info.device_id,
                           device_list[idx].acting_stream_id);
      device_list[idx].acting_stream_id = -1;
      break;
    default:
      ALOGD("Capture event is not supported");
      break;
  }
}

uint8_t
tv_input_hal_init()
{
  module = NULL;
  device = NULL;
  device_num = 0;
  device_list[MAX_DEVICE_NUM];

  tv_callback.notify = &device_init_notify;

  int err = hw_get_module(TV_INPUT_HARDWARE_MODULE_ID,
                          (hw_module_t const**)&module);
  if (err) {
    ALOGE("Couldn't load %s module (%d)",
           TV_INPUT_HARDWARE_MODULE_ID, err);
    return TV_STATUS_FAIL;
  }

  err = module->common.methods->open((hw_module_t*)module,
                                      TV_INPUT_DEFAULT_DEVICE,
                                      (hw_device_t**)&device);
  if(err) {
    ALOGE("Couldn't open %s device %d", TV_INPUT_DEFAULT_DEVICE, err);
    return TV_STATUS_FAIL;
  }

  device->initialize(device, &tv_callback, NULL);
  if (err) {
    ALOGE("Device initialization fail (%d)", err);
    return TV_STATUS_FAIL;
  }

  return TV_STATUS_SUCCESS;
}

uint8_t
tv_input_hal_uninit()
{
  uint8_t idx;
  for (idx = 0; idx < device_num; idx++) {
    if (device_list[idx].acting_stream_id != -1) {
      device->close_stream(device,
                           device_list[idx].device_id,
                           device_list[idx].acting_stream_id);
    }
  }
  return TV_STATUS_SUCCESS;
}

uint32_t
tv_input_hal_get_input_num_by_type(const int type)
{
  uint8_t idx;
  uint32_t sum;
  sum = 0;
  for (idx = 0; idx < device_num; idx++) {
    if (device_list[idx].type == type) {
      sum++;
    }
  }

  return sum;
}

uint8_t
tv_input_hal_get_inputs_by_type(const uint8_t input_num,
                                const int type, int* devices)
{
  uint8_t idx;
  uint8_t device_cnt;

  device_cnt = 0;

  for (idx = 0; idx < device_num; idx++) {
    if (device_list[idx].type == type) {
      devices[device_cnt++] = device_list[idx].device_id;
    }
  }

  return TV_STATUS_SUCCESS;
}

uint8_t
tv_input_hal_get_stream(int32_t device_id, tv_stream_t* tv_stream)
{
  /* Need to configure stream */
  int numConfigs;
  int result;
  const tv_stream_config_t* configs;
  int32_t idx;
  int configIndex;

  result = TV_STATUS_SUCCESS;
  numConfigs = 0;
  configIndex = -1;

  if (device->get_stream_configurations(
      device, device_id, &numConfigs, &configs) != 0) {
    ALOGE("Couldn't get stream configs");
    return TV_STATUS_FAIL;
  }
  /*
   * FIXME
   * We only handle independent video source in current phase.
   */
  for (idx = 0; idx < numConfigs; ++idx) {
    if (configs[idx].type == TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE) {
      configIndex = idx;
      break;
    }
  }

  if (configIndex == -1) {
    ALOGE("Cannot find a config with given stream ID.");
    return TV_STATUS_FAIL;
  }

  tv_stream->type = configs[configIndex].type;

  tv_stream->stream_id = configs[configIndex].stream_id;
  if (tv_stream->type == TV_STREAM_TYPE_BUFFER_PRODUCER) {
    tv_stream->buffer_producer.width = configs[configIndex].max_video_width;
    tv_stream->buffer_producer.height = configs[configIndex].max_video_height;
  }

  for (idx = 0; idx < device_num; idx++) {
    if (device_list[idx].device_id == device_id) {
      if (device_list[idx].acting_stream_id != -1) {
        if ( device_list[idx].acting_stream_id != tv_stream->stream_id) {
          result = device->close_stream(device,
                                        device_list[idx].device_id,
                                        device_list[idx].acting_stream_id);
          if (result != TV_STATUS_SUCCESS) {
            return TV_STATUS_FAIL;
          }
        } else {
          return TV_STATUS_SUCCESS;
        }
      }
      break;
    }
  }

  if (device->open_stream(device, device_id, tv_stream) != 0) {
    ALOGE("Couldn't add stream");
    return TV_STATUS_FAIL;
  }
  device_list[idx].acting_stream_id = tv_stream->stream_id;

  return TV_STATUS_SUCCESS;
}
