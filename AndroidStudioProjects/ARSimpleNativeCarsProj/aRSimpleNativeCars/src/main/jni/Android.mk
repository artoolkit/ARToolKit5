#--------------------------------------------------------------------------
#
#  ARSimpleNativeCars
#  ARToolKit for Android
#
#  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
#  LLC ("Daqri") in consideration of your agreement to the following
#  terms, and your use, installation, modification or redistribution of
#  this Daqri software constitutes acceptance of these terms.  If you do
#  not agree with these terms, please do not use, install, modify or
#  redistribute this Daqri software.
#
#  In consideration of your agreement to abide by the following terms, and
#  subject to these terms, Daqri grants you a personal, non-exclusive
#  license, under Daqri's copyrights in this original Daqri software (the
#  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
#  Software, with or without modifications, in source and/or binary forms;
#  provided that if you redistribute the Daqri Software in its entirety and
#  without modifications, you must retain this notice and the following
#  text and disclaimers in all such redistributions of the Daqri Software.
#  Neither the name, trademarks, service marks or logos of Daqri LLC may
#  be used to endorse or promote products derived from the Daqri Software
#  without specific prior written permission from Daqri.  Except as
#  expressly stated in this notice, no other rights or licenses, express or
#  implied, are granted by Daqri herein, including but not limited to any
#  patent rights that may be infringed by your derivative works or by other
#  works in which the Daqri Software may be incorporated.
#
#  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
#  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
#  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
#  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
#  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
#
#  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
#  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
#  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
#  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
#  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#
#  Copyright 2015 Daqri, LLC.
#  Copyright 2011-2015 ARToolworks, Inc.
#
#  Author(s): Philip Lamb
#
#--------------------------------------------------------------------------
MY_LOCAL_PATH := $(call my-dir)
LOCAL_PATH := $(MY_LOCAL_PATH)

# Pull ARWrapper and any other required libs into the build
include $(CLEAR_VARS)
ARTOOLKIT_DIR := $(MY_LOCAL_PATH)/../../../../../../android
ARTOOLKIT_LIBDIR := $(call host-path, $(ARTOOLKIT_DIR)/obj/local/$(TARGET_ARCH_ABI))
ARTOOLKIT_LIBDIR2 := $(call host-path, $(ARTOOLKIT_DIR)/libs/$(TARGET_ARCH_ABI))
define add_static_module
	include $(CLEAR_VARS)
	LOCAL_MODULE:=$1
	LOCAL_SRC_FILES:=lib$1.a
	include $(PREBUILT_STATIC_LIBRARY)
endef
define add_shared_module
	include $(CLEAR_VARS)
	LOCAL_MODULE:=$1
	LOCAL_SRC_FILES:=lib$1.so
	include $(PREBUILT_SHARED_LIBRARY)
endef
MY_STATIC_LIBS := eden
LOCAL_PATH := $(ARTOOLKIT_LIBDIR)
$(foreach module,$(MY_STATIC_LIBS),$(eval $(call add_static_module,$(module))))

MY_SHARED_LIBS := ARWrapper
LOCAL_PATH := $(ARTOOLKIT_LIBDIR2)
$(foreach module,$(MY_SHARED_LIBS),$(eval $(call add_shared_module,$(module))))

LOCAL_PATH := $(MY_LOCAL_PATH)

# Android arvideo depends on CURL.
CURL_DIR := $(ARTOOLKIT_DIR)/jni/curl
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
LOCAL_MODULE := ARWrapperNativeCarsExample
LOCAL_SRC_FILES := ARWrapperNativeCarsExample.cpp

# Make sure DEBUG is defined for debug builds. (NDK already defines NDEBUG for release builds.)
ifeq ($(APP_OPTIM),debug)
    LOCAL_CPPFLAGS += -DDEBUG
endif

LOCAL_C_INCLUDES += $(ARTOOLKIT_DIR)/../include/android $(ARTOOLKIT_DIR)/../include
LOCAL_LDLIBS += -llog -lGLESv1_CM
LOCAL_STATIC_LIBRARIES += eden
LOCAL_SHARED_LIBRARIES += ARWrapper
#LOCAL_SHARED_LIBRARIES += $(CURL_LIBS)

include $(BUILD_SHARED_LIBRARY)
