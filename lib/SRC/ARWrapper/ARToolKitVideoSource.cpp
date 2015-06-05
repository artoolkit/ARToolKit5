/*
 *  ARToolKitVideoSource.cpp
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

#include <ARWrapper/ARToolKitVideoSource.h>
#include <ARWrapper/ARController.h>
#include <ARWrapper/Error.h>
#include <stdlib.h>
// Define constants for extensions which became core in OpenGL 1.2
#ifndef GL_VERSION_1_2
#  if GL_EXT_bgra
#    define GL_BGR							GL_BGR_EXT
#    define GL_BGRA							GL_BGRA_EXT
#  else
#    define GL_BGR							0x80E0
#    define GL_BGRA							0x80E1
#  endif
#  ifndef GL_APPLE_packed_pixels
#    define GL_UNSIGNED_INT_8_8_8_8			0x8035
#    define GL_UNSIGNED_INT_8_8_8_8_REV		0x8367
#  endif
#  if GL_EXT_packed_pixels
#    define GL_UNSIGNED_SHORT_5_6_5         GL_UNSIGNED_SHORT_5_6_5_EXT
#    define GL_UNSIGNED_SHORT_5_5_5_1       GL_UNSIGNED_SHORT_5_5_5_1_EXT
#    define GL_UNSIGNED_SHORT_4_4_4_4       GL_UNSIGNED_SHORT_4_4_4_4_EXT
#  else
#    define GL_UNSIGNED_SHORT_5_6_5         0x8363
#    define GL_UNSIGNED_SHORT_5_5_5_1       0x8034
#    define GL_UNSIGNED_SHORT_4_4_4_4       0x8033
#  endif
#  ifndef GL_EXT_abgr
#    define GL_ABGR_EXT                     0x8000
#  endif
#endif

#if !TARGET_PLATFORM_ANDROID

ARToolKitVideoSource::ARToolKitVideoSource() : VideoSource() {

    gVid = NULL;
}

const char* ARToolKitVideoSource::getName() {
	return "ARToolKit Video Source";
}

bool ARToolKitVideoSource::open() {

	ARController::logv("Opening ARToolKit video.");
    
    if (deviceState != DEVICE_CLOSED) {
        ARController::logv("Error: device is already open.");
        return false;
    }

	// Open the video path
    gVid = ar2VideoOpen(videoConfiguration);
    if (!gVid) {
		ARController::logv("arVideoOpen unable to open connection to camera using configuration '%s'.", videoConfiguration);
    	return false;
	}
	ARController::logv("Opened connection to camera using configuration '%s'.", videoConfiguration);
	
	deviceState = DEVICE_OPEN;
    
    // Find the size of the video
	if (ar2VideoGetSize(gVid, &videoWidth, &videoHeight) < 0) {
		ARController::logv("Error: unable to get video size");
        this->close();
		return false;
	}
	
	// Get the format in which the camera is returning pixels
	pixelFormat = ar2VideoGetPixelFormat(gVid);
	if (pixelFormat < 0 ) {
    	ARController::logv("Error: unable to get pixel format.");
        this->close();
		return false;
	}
    
	ARController::logv("Video %dx%d@%dBpp (%s)", videoWidth, videoHeight, arUtilGetPixelSize(pixelFormat), arUtilGetPixelFormatName(pixelFormat));

#ifndef _WINRT
    // Translate pixel format into OpenGL texture intformat, format, and type.
    switch (pixelFormat) {
        case AR_PIXEL_FORMAT_RGBA:
            glPixIntFormat = GL_RGBA;
            glPixFormat = GL_RGBA;
            glPixType = GL_UNSIGNED_BYTE;
            break;
        case AR_PIXEL_FORMAT_RGB:
            glPixIntFormat = GL_RGB;
            glPixFormat = GL_RGB;
            glPixType = GL_UNSIGNED_BYTE;
            break;
        case AR_PIXEL_FORMAT_BGRA:
            glPixIntFormat = GL_RGBA;
            glPixFormat = GL_BGRA;
            glPixType = GL_UNSIGNED_BYTE;
            break;
		case AR_PIXEL_FORMAT_ABGR:
            glPixIntFormat = GL_RGBA;
            glPixFormat = GL_ABGR_EXT;
            glPixType = GL_UNSIGNED_BYTE;
			break;
		case AR_PIXEL_FORMAT_ARGB:
				glPixIntFormat = GL_RGBA;
				glPixFormat = GL_BGRA;
#ifdef AR_BIG_ENDIAN
				glPixType = GL_UNSIGNED_INT_8_8_8_8_REV;
#else
				glPixType = GL_UNSIGNED_INT_8_8_8_8;
#endif
			break;
		case AR_PIXEL_FORMAT_BGR:
            glPixIntFormat = GL_RGB;
            glPixFormat = GL_BGR;
            glPixType = GL_UNSIGNED_BYTE;
            break;
        case AR_PIXEL_FORMAT_MONO:
        case AR_PIXEL_FORMAT_420v:
        case AR_PIXEL_FORMAT_420f:
        case AR_PIXEL_FORMAT_NV21:
            glPixIntFormat = GL_LUMINANCE;
            glPixFormat = GL_LUMINANCE;
            glPixType = GL_UNSIGNED_BYTE;
            break;
        case AR_PIXEL_FORMAT_RGB_565:
            glPixIntFormat = GL_RGB;
            glPixFormat = GL_RGB;
            glPixType = GL_UNSIGNED_SHORT_5_6_5;
            break;
        case AR_PIXEL_FORMAT_RGBA_5551:
            glPixIntFormat = GL_RGBA;
            glPixFormat = GL_RGBA;
            glPixType = GL_UNSIGNED_SHORT_5_5_5_1;
            break;
        case AR_PIXEL_FORMAT_RGBA_4444:
            glPixIntFormat = GL_RGBA;
            glPixFormat = GL_RGBA;
            glPixType = GL_UNSIGNED_SHORT_4_4_4_4;
            break;
        default:
            ARController::logv("Error: Unsupported pixel format.\n");
            this->close();
			return false;
            break;
    }
#endif // !_WINRT

#if TARGET_PLATFORM_IOS
    // Tell arVideo what the typical focal distance will be. Note that this does NOT
    // change the actual focus, but on devices with non-fixed focus, it lets arVideo
    // choose a better set of camera parameters.
    ar2VideoSetParami(gVid, AR_VIDEO_PARAM_IOS_FOCUS, AR_VIDEO_IOS_FOCUS_0_3M); // Default is 0.3 metres. See <AR/sys/videoiPhone.h> for allowable values.
#endif
    

    // Load the camera parameters, resize for the window and init.
    ARParam cparam;
    // Prefer internal camera parameters.
    if (ar2VideoGetCParam(gVid, &cparam) == 0) {
        ARController::logv("Using internal camera parameters.");
    } else {
        const char cparam_name_default[] = "camera_para.dat"; // Default name for the camera parameters.
        if (cameraParamBuffer) {
            if (arParamLoadFromBuffer(cameraParamBuffer, cameraParamBufferLen, &cparam) < 0) {
                ARController::logv("Error: Failed to load camera parameters from buffer");        
                this->close();
                return false;
            } else {
                ARController::logv("Camera parameters loaded from buffer");
            }
        } else {
            if (arParamLoad((cameraParam ? cameraParam : cparam_name_default), 1, &cparam) < 0) {
                ARController::logv("Error: Failed to load camera parameters %s", (cameraParam ? cameraParam : cparam_name_default));        
                this->close();
                return false;
            } else {
                ARController::logv("Camera parameters loaded from %s", (cameraParam ? cameraParam : cparam_name_default));
            }
        }
    }

    if (cparam.xsize != videoWidth || cparam.ysize != videoHeight) {
#ifdef DEBUG
        ARController::logv("*** Camera Parameter resized from %d, %d. ***\n", cparam.xsize, cparam.ysize);
#endif
        arParamChangeSize(&cparam, videoWidth, videoHeight, &cparam);
    }
	if (!(cparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET))) {
    	ARController::logv("Error: Failed to create camera parameters lookup table.");
        this->close();
		return false;
	}

	int err = ar2VideoCapStart(gVid);
	if (err != 0) {
        if (err == -2) {
            ARController::logv("Error starting video: device unavailable.", err);
            setError(ARW_ERROR_DEVICE_UNAVAILABLE);
        } else {
            ARController::logv("Error %d starting video.", err);
        }
        this->close();
		return false;		
	}

	ARController::logv("Video capture started.");

	deviceState = DEVICE_RUNNING;

	return true;

}



bool ARToolKitVideoSource::captureFrame() {

	if (deviceState == DEVICE_RUNNING) {

        AR2VideoBufferT *vbuff = ar2VideoGetImage(gVid);
        if (vbuff && vbuff->buff) {
			frameStamp++;
            frameBuffer = vbuff->buff;
            frameBuffer2 = (vbuff->bufPlaneCount == 2 ? vbuff->bufPlanes[1] : NULL);
            return true;
		}
	}

	return false;

}

bool ARToolKitVideoSource::close() {

    if (deviceState == DEVICE_CLOSED) return true;
    
	if (deviceState == DEVICE_RUNNING) {
		ARController::logv("Stopping video.");
		int err = ar2VideoCapStop(gVid);
		if (err != 0) ARController::logv("Error %d stopping video.", err);
        
        if (cparamLT) arParamLTFree(&cparamLT);
        
        deviceState = DEVICE_OPEN;
    }
    
    frameBuffer = NULL;
    frameBuffer2 = NULL;

    ARController::logv("Closing video.");
    if (ar2VideoClose(gVid) != 0) ARController::logv("Error closing video.");
	
    gVid = NULL;

	deviceState = DEVICE_CLOSED; // ARToolKit video source is always ready to be opened.

	return true;

}

#endif