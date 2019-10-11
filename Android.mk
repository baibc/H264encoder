LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CEDARX_PATH:= $(TOP)/frameworks/av/media/liballwinner/LIBRARY

include $(LOCAL_CEDARX_PATH)/config.mk

LOCAL_MODULE := vencoderTest

LOCAL_MODULE_TAGS := option

#LOCAL_SRC_FILES := \
	libve_enc.c \
	testENC.c

LOCAL_SRC_FILES := \
	libve_enc.cpp \
	usbcamera.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libVE \
	libMemAdapter \
	libvencoder \
	libbinder \
  	libui \
  	libgui \
  	libstagefright\
  	libstagefright_foundation

LOCAL_STATIC_LIBRARIES := \
  	libstagefright_color_conversion

LOCAL_C_INCLUDES := \
	$(LOCAL_CEDARX_PATH)/ \
	$(LOCAL_CEDARX_PATH)/CODEC/VIDEO/ENCODER/include \
	$(JNI_H_INCLUDE) \
    $(LOCAL_PATH)/include \
	frameworks/native/include/media/openmax \
	frameworks/av/media/libstagefright

include $(BUILD_EXECUTABLE)


