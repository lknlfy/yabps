LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := yabps
LOCAL_SRC_FILES := main.c
LOCAL_STATIC_LIBRARIES += rda5876 auth mm drv abcsp hci l2cap util connection
LOCAL_LDLIBS := -llog

include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under,$(LOCAL_PATH))
