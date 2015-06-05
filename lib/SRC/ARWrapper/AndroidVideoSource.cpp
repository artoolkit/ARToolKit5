/*
 *  AndroidVideoSource.cpp
 *  ARToolKit5
 *
 *  This file is part of ARToolKit.
 *
 *  ARToolKit is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ARToolKit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Julian Looser, Philip Lamb.
 *
 */

#include <ARWrapper/Platform.h>

#if TARGET_PLATFORM_ANDROID

#include <ARWrapper/AndroidVideoSource.h>
#include <ARWrapper/ColorConversion.h>
#include <jni.h>

#include <ARWrapper/ARController.h>

AndroidVideoSource::AndroidVideoSource() : VideoSource(),
    newFrameArrived(false),
    localFrameBuffer(NULL),
    frameBufferSize(0),
    gCameraIndex(0),
    gCameraIsFrontFacing(false) {
}

const char* AndroidVideoSource::getName() {
	return "Android Video Source";
}

bool AndroidVideoSource::open() {
    
	ARController::logv("Opening Android Video Source.");
    
    if (deviceState != DEVICE_CLOSED) {
        ARController::logv("Error: device is already open.");
        return false;
    }
    
	// On Android, ARVideo doesn't actually provide the frames, but it is needed to handle
    // fetching of the camera parameters. Note that if the current working directory
    // isn't already the directory where the camera parametere cache should be created,
    // then the videoconfiguration should include the option 'cachedir="/path/to/cache"'.
    gVid = ar2VideoOpen(videoConfiguration);
    if (!gVid) {
		ARController::logv("arVideoOpen unable to open connection to camera.");
    	return false;
	}
	//ARController::logv("Opened connection to camera.");

    pixelFormat = ar2VideoGetPixelFormat(gVid);
    if (pixelFormat == AR_PIXEL_FORMAT_INVALID) {
        ARController::logv("AndroidVideoSource::getVideoReadyAndroid: Error: No pixel format set.\n");
        goto bail;
    }
    
	deviceState = DEVICE_OPEN;
	return true;
    
bail:
    ar2VideoClose(gVid);
    gVid = NULL;
    return false;
}

bool AndroidVideoSource::getVideoReadyAndroid(const int width, const int height, const int cameraIndex, const bool cameraIsFrontFacing) {
	
    char *a, b[1024];
    int err_i;
    
    if (deviceState == DEVICE_GETTING_READY) return true;
    else if (deviceState != DEVICE_OPEN) {
        ARController::logv("AndroidVideoSource::getVideoReadyAndroid: Error: device not open.\n");
        return false;
    }
    deviceState = DEVICE_GETTING_READY;

#ifdef DEBUG
    ARController::logv("AndroidVideoSource::getVideoReadyAndroid: width=%d, height=%d, cameraIndex=%d, cameraIsFrontFacing=%s.\n", width, height, cameraIndex, (cameraIsFrontFacing ? "true" : "false"));
#endif
    
	videoWidth = width;
	videoHeight = height;
    gCameraIndex = cameraIndex;
    gCameraIsFrontFacing = cameraIsFrontFacing;

    if (pixelFormat == AR_PIXEL_FORMAT_RGBA) {
        glPixIntFormat = GL_RGBA;
        glPixFormat = GL_RGBA;
        glPixType = GL_UNSIGNED_BYTE;
    } else if (pixelFormat == AR_PIXEL_FORMAT_NV21 || pixelFormat == AR_PIXEL_FORMAT_420f) {
        glPixIntFormat = GL_LUMINANCE; // Use only luma channel.
        glPixFormat = GL_LUMINANCE;
        glPixType = GL_UNSIGNED_BYTE;
    } else {
        ARController::logv("Unsupported video format '%s'.\n", arUtilGetPixelFormatName(pixelFormat));
        return false;
    }
    
    ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_WIDTH, videoWidth);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_HEIGHT, videoHeight);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_CAMERA_INDEX, gCameraIndex);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_CAMERA_FACE, gCameraIsFrontFacing);
	//ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_INTERNET_STATE, gInternetState);

	if (ar2VideoGetCParamAsync(gVid, getVideoReadyAndroidCparamCallback, (void *)this) < 0) {
		ARController::logv("Error getting cparam.\n");
		getVideoReadyAndroid2(NULL);
	}
    
	return (true);
}

// static callback method.
void AndroidVideoSource::getVideoReadyAndroidCparamCallback(const ARParam *cparam_p, void *userdata)
{
    if (!userdata) return;
    AndroidVideoSource *vs = reinterpret_cast<AndroidVideoSource *>(userdata);
    vs->getVideoReadyAndroid2(cparam_p);
}

bool AndroidVideoSource::getVideoReadyAndroid2(const ARParam *cparam_p) {
    
	// Load camera parameters
    ARParam cparam;
	if (cparam_p) cparam = *cparam_p;
	else {
	    ARController::logv("Unable to automatically determine camera parameters. Using supplied default.\n");
        if (cameraParam) {
            if (arParamLoad(cameraParam, 1, &cparam) < 0) {
                ARController::logv("Error: Unable to load camera parameters from file '%s'.", cameraParam);
                goto bail;
            }
        } else if (cameraParamBuffer) {
            if (arParamLoadFromBuffer(cameraParamBuffer, cameraParamBufferLen, &cparam) < 0) {
                ARController::logv("Error: Unable to load camera parameters from buffer.");
                goto bail;
            }
        } else {
            ARController::logv("Error: video source must be configured before opening.");
            goto bail;
        }
	}

	if (cparam.xsize != videoWidth || cparam.ysize != videoHeight) {
#ifdef DEBUG
        ARController::logv("*** Camera Parameter resized from %d, %d. ***", cparam.xsize, cparam.ysize);
#endif
        arParamChangeSize(&cparam, videoWidth, videoHeight, &cparam);
    }
	if (!(cparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET))) {
    	ARController::logv("Error: Failed to create camera parameters lookup table.");
        goto bail;
	}

	// Allocate local buffer for video frame after copy or conversion.
    if (pixelFormat == AR_PIXEL_FORMAT_NV21 || pixelFormat == AR_PIXEL_FORMAT_420f) {
        frameBufferSize = videoWidth * videoHeight + 2 * videoWidth/2 * videoHeight/2;
    } else {
        frameBufferSize = videoWidth * videoHeight * arUtilGetPixelSize(pixelFormat);
    }
    localFrameBuffer = (ARUint8*)calloc(frameBufferSize, sizeof(ARUint8));
	if (!localFrameBuffer) {
        ARController::logv("Error: Unable to allocate memory for local video frame buffer");
        goto bail;
	}
    frameBuffer = localFrameBuffer;
    if (pixelFormat == AR_PIXEL_FORMAT_NV21 || pixelFormat == AR_PIXEL_FORMAT_420f) {
        frameBuffer2 = localFrameBuffer + videoWidth*videoHeight;
    } else {
        frameBuffer2 = NULL;
    }
    

	ARController::logv("Android Video Source running %dx%d.", videoWidth, videoHeight);

	deviceState = DEVICE_RUNNING;
    return true;
    
bail:
    deviceState = DEVICE_OPEN;
    return false;
}

size_t AndroidVideoSource::getFrameSize() {
    return frameBufferSize;
}

bool AndroidVideoSource::captureFrame() {

    //ARController::logv("AndroidVideoSource::captureFrame()");
    if (deviceState == DEVICE_RUNNING) {
        
        if (newFrameArrived) {
            newFrameArrived = false;
            return true;
        }
    }
    
    return false;
}

void AndroidVideoSource::acceptImage(ARUint8* ptr) {
	
    //ARController::logv("AndroidVideoSource::acceptImage()");
	if (deviceState == DEVICE_RUNNING) {
        if (pixelFormat == AR_PIXEL_FORMAT_NV21 || pixelFormat == AR_PIXEL_FORMAT_420f) {
            // Nothing more to do.
        } else if (ptr && pixelFormat == AR_PIXEL_FORMAT_RGBA) {
            color_convert_common((unsigned char*)ptr, (unsigned char*)(ptr + videoWidth * videoHeight), videoWidth, videoHeight, localFrameBuffer);
            
        } else {
            return;
        }
		frameStamp++;
		newFrameArrived = true;
    }
}

bool AndroidVideoSource::close() {

    if (deviceState == DEVICE_CLOSED) return true;
    
    if (cparamLT) arParamLTFree(&cparamLT);

	if (localFrameBuffer) {
		free(localFrameBuffer);
		localFrameBuffer = NULL;
        frameBuffer = NULL;
        frameBuffer2 = NULL;
        frameBufferSize = 0;
	}
    newFrameArrived = false;
    ar2VideoClose(gVid);
    gVid = NULL;

	deviceState = DEVICE_CLOSED;
	ARController::logv("Android Video Source closed.");
    
    return true;
}

#endif