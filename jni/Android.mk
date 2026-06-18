LOCAL_PATH := /root/Trigger-SKRoot

# 预构建 SKRoot SDK 静态库
include $(CLEAR_VARS)
LOCAL_MODULE := kernel_module_kit_static
LOCAL_SRC_FILES := $(LOCAL_PATH)/skroot_sdk/lib/libkernel_module_kit_static.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/skroot_sdk/include
include $(PREBUILT_STATIC_LIBRARY)

# 主模块
include $(CLEAR_VARS)
LOCAL_MODULE := trigger_skroot
LOCAL_SRC_FILES := $(LOCAL_PATH)/trigger_skroot.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/skroot_sdk/include
LOCAL_STATIC_LIBRARIES := kernel_module_kit_static

COMMON_CPPFLAGS := \
    -std=c++20 \
    -fPIC \
    -fvisibility=hidden \
    -fno-stack-protector \
    -ffunction-sections \
    -fdata-sections \
    -flto=thin

COMMON_LDFLAGS := \
    -Wl,-O2 \
    -Wl,--gc-sections \
    -Wl,-s \
    -flto=thin

LOCAL_CPPFLAGS += $(COMMON_CPPFLAGS)
LOCAL_LDFLAGS += $(COMMON_LDFLAGS)

include $(BUILD_SHARED_LIBRARY)
