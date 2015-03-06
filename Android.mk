
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
CEDARX_TOP := $(TOP)/frameworks/av/media/CedarX-Projects/CedarX
LOCAL_MODULE := hawkview
#LOCAL_STATIC_LIBRARIES := libsc
#LOCAL_LDLIBS := -llog -lm
LOCAL_CFLAGS := -DANDROID_ENV
LOCAL_SRC_FILES := main.c \
                   common/hawkview.c \
                   common/video_helper.c \
                   common/sun9iw1p1_video.c \
                   common/jpeg_encoder.c \
                   platform/sun9iw1p1/sun9iw1p1_disp.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/common \
                    $(LOCAL_PATH)/platform\sun9iw1p1 \
                    $(CEDARX_TOP)/include \
                    $(CEDARX_TOP)/include/include_camera   
LOCAL_SHARED_LIBRARIES := liblog \
			  libcutils \
			  libjpgenc \
			  libcedarxosal \
			  libcedarxbase \
			  libion_alloc
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)



