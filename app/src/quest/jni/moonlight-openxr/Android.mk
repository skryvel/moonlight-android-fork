# Android.mk for Quest OpenXR support
# This module is only built for the Quest flavor
LOCAL_PATH := $(call my-dir)

$(info [Moonlight Quest] Processing Quest OpenXR Android.mk)
$(info [Moonlight Quest] LOCAL_PATH = $(LOCAL_PATH))
$(info [Moonlight Quest] HAS_OPENXR = $(HAS_OPENXR))

# Only build this module if HAS_OPENXR is defined (Quest builds)
ifeq ($(HAS_OPENXR),1)

$(info [Moonlight Quest] Building libmoonlight-openxr.so)

include $(CLEAR_VARS)
LOCAL_MODULE    := moonlight-openxr

# OpenXR source files
LOCAL_SRC_FILES := ../../../main/jni/moonlight-core/openxr_input.c \
                   ../../../main/jni/moonlight-core/openxr_jni.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../main/jni/moonlight-core

LOCAL_CFLAGS := -DHAS_OPENXR=1

LOCAL_LDLIBS := -llog -landroid -lopenxr_loader

include $(BUILD_SHARED_LIBRARY)

endif # HAS_OPENXR

