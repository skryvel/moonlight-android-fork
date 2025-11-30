# Include all subdirectories in main jni folder
include $(call all-subdir-makefiles)

# For Quest builds, also include the Quest-specific OpenXR module
# Use -include to silently skip if file doesn't exist (for non-Quest source trees)
QUEST_OPENXR_MK := $(LOCAL_PATH)/../../quest/jni/moonlight-openxr/Android.mk
-include $(QUEST_OPENXR_MK)

