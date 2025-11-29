# Android.mk for Quest OpenXR support
# This module is only built for the Quest flavor
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := moonlight-openxr

# OpenXR source files
LOCAL_SRC_FILES := ../../../main/jni/moonlight-core/openxr_input.c \
                   ../../../main/jni/moonlight-core/openxr_jni.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../main/jni/moonlight-core

LOCAL_CFLAGS := -DHAS_OPENXR=1

LOCAL_LDLIBS := -llog -landroid -lopenxr_loader

include $(BUILD_SHARED_LIBRARY)
