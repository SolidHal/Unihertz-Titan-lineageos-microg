LOCAL_PATH := $(call my-dir)

ifneq ($(filter titan,$(TARGET_DEVICE)),)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
