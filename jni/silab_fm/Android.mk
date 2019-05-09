# Copyright (C) 2019 Draekko, Benoit Touchette
# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifneq ($(BOARD_HAS_SILAB_FM),true)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    fmr_err.cpp \
    libfmsilab_jni.cpp \
    silab_ioctl.cpp

LOCAL_C_INCLUDES := $(JNI_H_INCLUDE) \
    frameworks/base/include/media

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libmedia \
    liblog

LOCAL_CFLAGS += -Wno-error
LOCAL_CXXFLAGS += -Wno-error


LOCAL_MODULE := libfmsilab_jni
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
endif # BOARD_HAS_SILAB_FM
