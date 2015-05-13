# In order to compile your application under cygwin
# you might need to define NDK_USE_CYGPATH=1 before calling the ndk-build

USER_LOCAL_PATH:=$(LOCAL_PATH)
LOCAL_PATH:=$(subst ?,,$(firstword ?$(subst \, ,$(subst /, ,$(call my-dir)))))

OPENCV_TARGET_ARCH_ABI:=$(TARGET_ARCH_ABI)
OPENCV_THIS_DIR:=$(patsubst $(LOCAL_PATH)\\%,%,$(patsubst $(LOCAL_PATH)/%,%,$(call my-dir)))
OPENCV_MK_DIR:=$(dir $(lastword $(MAKEFILE_LIST)))
OPENCV_LIBS_DIR:=$(OPENCV_THIS_DIR)/../libs/$(OPENCV_TARGET_ARCH_ABI)
OPENCV_3RDPARTY_LIBS_DIR:=$(OPENCV_THIS_DIR)/../3rdparty/libs/$(OPENCV_TARGET_ARCH_ABI)
OPENCV_BASEDIR:=
OPENCV_LOCAL_C_INCLUDES:="$(LOCAL_PATH)/$(OPENCV_THIS_DIR)/include/opencv" "$(LOCAL_PATH)/$(OPENCV_THIS_DIR)/include"
# ARToolKit needs only flann and core for NFT apps, and calib3d, features2d, imgproc and core for camera calibration.
#OPENCV_MODULES:=contrib legacy ml stitching objdetect ts videostab calib3d photo video features2d highgui androidcamera flann imgproc core
OPENCV_MODULES:=calib3d features2d flann imgproc core

ifeq ($(OPENCV_LIB_TYPE),)
    OPENCV_LIB_TYPE:=SHARED
endif

ifeq ($(OPENCV_LIB_TYPE),SHARED)
    OPENCV_LIBS:=java
    OPENCV_LIB_TYPE:=SHARED
else
    OPENCV_LIBS:=$(OPENCV_MODULES)
    OPENCV_LIB_TYPE:=STATIC
endif

ifeq ($(OPENCV_LIB_TYPE),SHARED)
    OPENCV_3RDPARTY_COMPONENTS:=
    OPENCV_EXTRA_COMPONENTS:=
else
# ARToolKit does not use highgui, so no need for external file format handling libs.
#    OPENCV_3RDPARTY_COMPONENTS:=libjpeg libpng libtiff libjasper IlmImf
    OPENCV_3RDPARTY_COMPONENTS:=
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    OPENCV_3RDPARTY_COMPONENTS+=tbb
endif
ifeq ($(TARGET_ARCH_ABI),x86)
    OPENCV_3RDPARTY_COMPONENTS+=tbb
endif
    OPENCV_EXTRA_COMPONENTS:=c log m dl z
endif

ifeq (${OPENCV_CAMERA_MODULES},on)
    ifeq ($(TARGET_ARCH_ABI),armeabi)
        OPENCV_CAMERA_MODULES:= native_camera_r2.2.0 native_camera_r2.3.3 native_camera_r3.0.1 native_camera_r4.0.0 native_camera_r4.0.3 native_camera_r4.1.1
    endif
    ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
        OPENCV_CAMERA_MODULES:= native_camera_r2.2.0 native_camera_r2.3.3 native_camera_r3.0.1 native_camera_r4.0.0 native_camera_r4.0.3 native_camera_r4.1.1
    endif
    ifeq ($(TARGET_ARCH_ABI),x86)
        OPENCV_CAMERA_MODULES:= native_camera_r2.3.3 native_camera_r3.0.1 native_camera_r4.0.3 native_camera_r4.1.1
    endif
else
    OPENCV_CAMERA_MODULES:=
endif

ifeq ($(OPENCV_LIB_TYPE),SHARED)
    OPENCV_LIB_SUFFIX:=so
else
    OPENCV_LIB_SUFFIX:=a
    OPENCV_INSTALL_MODULES:=on
endif

define add_opencv_module
    include $(CLEAR_VARS)
    LOCAL_MODULE:=opencv_$1
    LOCAL_SRC_FILES:=$(OPENCV_LIBS_DIR)/libopencv_$1.$(OPENCV_LIB_SUFFIX)
    include $(PREBUILT_$(OPENCV_LIB_TYPE)_LIBRARY)
endef

define add_opencv_3rdparty_component
    include $(CLEAR_VARS)
    LOCAL_MODULE:=$1
    LOCAL_SRC_FILES:=$(OPENCV_3RDPARTY_LIBS_DIR)/lib$1.a
    include $(PREBUILT_STATIC_LIBRARY)
endef

define add_opencv_camera_module
    include $(CLEAR_VARS)
    LOCAL_MODULE:=$1
    LOCAL_SRC_FILES:=$(OPENCV_LIBS_DIR)/lib$1.so
    include $(PREBUILT_SHARED_LIBRARY)
endef

ifeq ($(OPENCV_INSTALL_MODULES),on)
$(foreach module,$(OPENCV_LIBS),$(eval $(call add_opencv_module,$(module))))
endif
$(foreach module,$(OPENCV_3RDPARTY_COMPONENTS),$(eval $(call add_opencv_3rdparty_component,$(module))))
$(foreach module,$(OPENCV_CAMERA_MODULES),$(eval $(call add_opencv_camera_module,$(module))))

ifneq ($(OPENCV_BASEDIR),)
    OPENCV_LOCAL_C_INCLUDES += $(foreach mod, $(OPENCV_MODULES), $(OPENCV_BASEDIR)/modules/$(mod)/include)
endif

ifeq ($(OPENCV_LOCAL_CFLAGS),)
    OPENCV_LOCAL_CFLAGS := -fPIC -DANDROID -fsigned-char
endif

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(OPENCV_LOCAL_C_INCLUDES)
LOCAL_CFLAGS     += $(OPENCV_LOCAL_CFLAGS)

ifeq ($(OPENCV_INSTALL_MODULES),on)
    LOCAL_$(OPENCV_LIB_TYPE)_LIBRARIES += $(foreach mod, $(OPENCV_LIBS), opencv_$(mod))
else
    LOCAL_LDLIBS += -L$(call host-path,$(LOCAL_PATH)/$(OPENCV_LIBS_DIR)) $(foreach lib, $(OPENCV_LIBS), -lopencv_$(lib))
endif

ifeq ($(OPENCV_LIB_TYPE),STATIC)
    LOCAL_STATIC_LIBRARIES += $(OPENCV_3RDPARTY_COMPONENTS)
endif

LOCAL_LDLIBS += $(foreach lib,$(OPENCV_EXTRA_COMPONENTS), -l$(lib))

#restore the LOCAL_PATH
LOCAL_PATH:=$(USER_LOCAL_PATH)
