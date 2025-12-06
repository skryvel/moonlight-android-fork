# Android.mk for Quest OpenXR support
# This module is only built when HAS_OPENXR=1 is passed to ndk-build
LOCAL_PATH := $(call my-dir)

# Debug output
$(info ========================================)
$(info [Quest OpenXR] Processing Android.mk)
$(info [Quest OpenXR] LOCAL_PATH = $(LOCAL_PATH))
$(info [Quest OpenXR] HAS_OPENXR = $(HAS_OPENXR))
$(info ========================================)

# Only build this module if HAS_OPENXR=1
ifeq ($(HAS_OPENXR),1)

$(info [Quest OpenXR] ✓ Building libmoonlight-openxr.so)

# Prebuilt OpenXR loader library (from AAR dependency)
include $(CLEAR_VARS)
LOCAL_MODULE := openxr_loader
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libopenxr_loader.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(PREBUILT_SHARED_LIBRARY)

# Our OpenXR wrapper module
include $(CLEAR_VARS)
LOCAL_MODULE    := moonlight-openxr

# OpenXR source files (relative to this Android.mk location)
LOCAL_SRC_FILES := ../moonlight-core/openxr_input.c \
                   ../moonlight-core/openxr_jni.c

# Include paths for OpenXR headers and moonlight-core
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
                    $(LOCAL_PATH)/../moonlight-core

LOCAL_CFLAGS := -DHAS_OPENXR=1 -DXR_USE_PLATFORM_ANDROID -DXR_USE_GRAPHICS_API_OPENGL_ES

LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3

# Link against the prebuilt OpenXR loader
LOCAL_SHARED_LIBRARIES := openxr_loader

include $(BUILD_SHARED_LIBRARY)

else

$(info [Quest OpenXR] ✗ Skipping build - HAS_OPENXR not set to 1)

endif # HAS_OPENXR
