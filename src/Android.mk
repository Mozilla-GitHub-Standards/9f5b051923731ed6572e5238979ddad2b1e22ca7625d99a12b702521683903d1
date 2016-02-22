LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= dtv.c \
                  dtv_pdu.c \
                  dtv_io.c \
                  tv_hal.c \
                  tv_utils.c \
                  io.c \
                  main.c \
                  pdu.c \
                  registry.c \
                  service.c
LOCAL_C_INCLUDES := system/libfdio/include \
                    system/libpdu/include
LOCAL_CFLAGS := -DANDROID_VERSION=$(PLATFORM_SDK_VERSION) -Wall
LOCAL_SHARED_LIBRARIES := libpdu \
                          libfdio \
                          libhardware \
                          libhardware_legacy \
                          liblog
LOCAL_MODULE:= tvd
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
