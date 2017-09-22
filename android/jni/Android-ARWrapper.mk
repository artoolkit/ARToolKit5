#--------------------------------------------------------------------------
#
#  ARToolKit5
#  ARToolKit for Android
#
#  This file is part of ARToolKit.
#
#  ARToolKit is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  ARToolKit is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
#
#  As a special exception, the copyright holders of this library give you
#  permission to link this library with independent modules to produce an
#  executable, regardless of the license terms of these independent modules, and to
#  copy and distribute the resulting executable under terms of your choice,
#  provided that you also meet, for each linked independent module, the terms and
#  conditions of the license of that module. An independent module is a module
#  which is neither derived from nor based on this library. If you modify this
#  library, you may extend this exception to your version of the library, but you
#  are not obligated to do so. If you do not wish to do so, delete this exception
#  statement from your version.
#
#  Copyright 2015-2016 Daqri, LLC.
#  Copyright 2011-2015 ARToolworks, Inc.
#
#  Authors: Julian Looser, Philip Lamb
#
#--------------------------------------------------------------------------

MY_LOCAL_PATH := $(call my-dir)

NDK11_CHK2:=$(shell $(MY_LOCAL_PATH)/assert_ndk_version.sh r11)
ifeq ($(NDK11_CHK2),false)
    $(error NDK version r11 or greater required)
endif

#
# Local variables: MY_CFLAGS, MY_FILES
#

# Make sure DEBUG is defined for debug builds. (NDK already defines NDEBUG for release builds.)
MY_CFLAGS :=
ifeq ($(APP_OPTIM),debug)
    $(info Android-ARWrapper.mk Start ARWrapper build ($(TARGET_ARCH_ABI) DEBUG))
    MY_CFLAGS += -DDEBUG
else
    $(info Android-ARWrapper.mk Start ARWrapper build ($(TARGET_ARCH_ABI) $(APP_OPTIM)))
endif

ARTOOLKIT_ROOT := $(MY_LOCAL_PATH)/../..

#--------------------------------------------------------------------------
# libARWrapper
#--------------------------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_PATH := $(MY_LOCAL_PATH)

# Pull other ARToolKit libs into the build
ARTOOLKIT_LIBDIR := $(call host-path, $(LOCAL_PATH)/../obj/local/$(TARGET_ARCH_ABI))
define add_artoolkit_module
	include $(CLEAR_VARS)
	LOCAL_MODULE:=$1
	LOCAL_SRC_FILES:=lib$1.a
	include $(PREBUILT_STATIC_LIBRARY)
endef
ARTOOLKIT_LIBS := ar2 kpm argsub_es armulti ar aricp arvideo arutil jpeg
LOCAL_PATH := $(ARTOOLKIT_LIBDIR)
$(foreach module,$(ARTOOLKIT_LIBS),$(eval $(call add_artoolkit_module,$(module))))
LOCAL_PATH := $(MY_LOCAL_PATH)

# Android arvideo depends on CURL.
CURL_DIR := $(ARTOOLKIT_ROOT)/android/jni/curl
CURL_LIBDIR := $(call host-path, $(CURL_DIR)/libs/$(TARGET_ARCH_ABI))
define add_curl_module
	include $(CLEAR_VARS)
	LOCAL_MODULE:=$1
	#LOCAL_SRC_FILES:=lib$1.so
	#include $(PREBUILT_SHARED_LIBRARY)
	LOCAL_SRC_FILES:=lib$1.a
	include $(PREBUILT_STATIC_LIBRARY)
endef
#CURL_LIBS := curl ssl crypto
CURL_LIBS := curl
LOCAL_PATH := $(CURL_LIBDIR)
$(foreach module,$(CURL_LIBS),$(eval $(call add_curl_module,$(module))))
LOCAL_PATH := $(MY_LOCAL_PATH)

include $(CLEAR_VARS)

# ARToolKit libs use lots of floating point, so don't compile in thumb mode.
LOCAL_ARM_MODE := arm

LOCAL_PATH := $(MY_LOCAL_PATH)
LOCAL_MODULE := ARWrapper
MY_FILES := $(wildcard $(ARTOOLKIT_ROOT)/lib/SRC/ARWrapper/*.c*)
MY_FILES := $(MY_FILES:$(LOCAL_PATH)/%=%)
# Rather than using LOCAL_ARM_NEON := true, just compile the one file in NEON mode.
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  MY_FILES := $(subst ARVideoSource.cpp,ARVideoSource.cpp.neon,$(MY_FILES))
  LOCAL_CFLAGS += -DHAVE_ARM_NEON=1
endif
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
  MY_FILES := $(subst ARVideoSource.cpp,ARVideoSource.cpp.neon,$(MY_FILES))
  LOCAL_CFLAGS += -DHAVE_ARM64_NEON=1
endif
LOCAL_SRC_FILES := $(MY_FILES)

LOCAL_C_INCLUDES += $(ARTOOLKIT_ROOT)/include/android $(ARTOOLKIT_ROOT)/include
LOCAL_CFLAGS += -DHAVE_NFT=1 $(MY_CFLAGS)
LOCAL_LDLIBS += -llog -lGLESv1_CM -lz
LOCAL_WHOLE_STATIC_LIBRARIES += ar
LOCAL_STATIC_LIBRARIES += ar2 kpm argsub_es armulti aricp arvideo arutil jpeg cpufeatures
#LOCAL_SHARED_LIBRARIES += $(CURL_LIBS)
LOCAL_STATIC_LIBRARIES += $(CURL_LIBS)

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)
