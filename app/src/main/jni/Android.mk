# Include all subdirectories in main jni folder
include $(call all-subdir-makefiles)

# For Quest builds, also include the Quest-specific OpenXR module
# This must be done AFTER all-subdir-makefiles to avoid conflicts
MY_LOCAL_PATH := $(call my-dir)
QUEST_OPENXR_MK := $(MY_LOCAL_PATH)/../../quest/jni/moonlight-openxr/Android.mk

# Only include if the file exists (Quest source tree)
ifneq ($(wildcard $(QUEST_OPENXR_MK)),)
    $(info [Moonlight] Including Quest OpenXR module: $(QUEST_OPENXR_MK))
    $(info [Moonlight] HAS_OPENXR = $(HAS_OPENXR))
    include $(QUEST_OPENXR_MK)
else
    $(info [Moonlight] Quest OpenXR module not found, skipping)
endif

