/*
 *  VideoSource.cpp
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
#include <ARWrapper/VideoSource.h>
#include <ARWrapper/Error.h>
#if TARGET_PLATFORM_ANDROID
#  include <ARWrapper/AndroidVideoSource.h>
#endif
#include <ARWrapper/ARToolKitVideoSource.h>
#include <ARWrapper/ARController.h>
#include <ARWrapper/ColorConversion.h>

#define MAX(x,y) (x > y ? x : y)
#define MIN(x,y) (x < y ? x : y)
#define CLAMP(x,r1,r2) (MIN(MAX(x,r1),r2))

VideoSource* VideoSource::newVideoSource()
{    
#if TARGET_PLATFORM_ANDROID
	return new AndroidVideoSource();
#else
	return new ARToolKitVideoSource();
#endif
    
}

VideoSource::VideoSource() : 
    deviceState(DEVICE_CLOSED),
	cameraParam(NULL),
    cameraParamBuffer(NULL),
    cameraParamBufferLen(0L),
    cparamLT(NULL),
    videoConfiguration(NULL),
    videoWidth(0),
    videoHeight(0),
    pixelFormat((AR_PIXEL_FORMAT)(-1)),
#ifndef _WINRT
    glPixIntFormat(0),
    glPixFormat(0),
    glPixType(0),
#endif
    frameBuffer(NULL),
    frameBuffer2(NULL),
    frameStamp(0),
    m_error(ARW_ERROR_NONE)
{
        
}

VideoSource::~VideoSource() {

	if (videoConfiguration) {
		free(videoConfiguration);
		videoConfiguration = NULL;
	}

	if (cameraParam) {
		free(cameraParam);
		cameraParam = NULL;
	}

	if (cameraParamBuffer) {
		free(cameraParamBuffer);
		cameraParamBuffer = NULL;
	}

}

void VideoSource::setError(int error)
{
    if (m_error == ARW_ERROR_NONE) {
        m_error = error;
    }
}

int VideoSource::getError()
{
    int temp = m_error;
    if (temp != ARW_ERROR_NONE) {
        m_error = ARW_ERROR_NONE;
    }
    return temp;
}

void VideoSource::configure(const char* vconf, const char* cparaName, const char* cparaBuff, size_t cparaBuffLen) {

	if (vconf) {
		size_t len = strlen(vconf);
		videoConfiguration = (char*)malloc(sizeof(char) * len + 1);
		strcpy(videoConfiguration, vconf);
		ARController::logv("Video Source video configuration: %s", videoConfiguration);
	}

	if (cparaName) {
		size_t len = strlen(cparaName);
		cameraParam = (char*)malloc(sizeof(char) * len + 1);
		strcpy(cameraParam, cparaName);
		ARController::logv("Video Source camera parameters: %s", cameraParam);
	}

	if (cparaBuff) {
		cameraParamBufferLen = cparaBuffLen;
		cameraParamBuffer = (char*)malloc(sizeof(char) * cameraParamBufferLen);
		memcpy(cameraParamBuffer, cparaBuff, cameraParamBufferLen);
		ARController::logv("Video Source camera parameters buffer: %ld bytes", cameraParamBufferLen);
	}
    
}

bool VideoSource::isOpen() {
	return deviceState != DEVICE_CLOSED;
}

bool VideoSource::isRunning() {
	return deviceState == DEVICE_RUNNING;
}


ARParamLT* VideoSource::getCameraParameters() {
	return cparamLT;
}

int VideoSource::getVideoWidth() {
	return videoWidth;
}
	
int VideoSource::getVideoHeight() {
	return videoHeight;
}

AR_PIXEL_FORMAT VideoSource::getPixelFormat() {
	return pixelFormat;
}

ARUint8* VideoSource::getFrame() {
	return frameBuffer;
}

int VideoSource::getFrameStamp() {
	return frameStamp;
}

bool VideoSource::updateTexture(Color* buffer) {
	
	static int lastFrameStamp = 0;

    if (!buffer) return false; // Sanity check.
    
    if (!frameBuffer) return false; // Check that a frame is actually available.
	
    // Extra check: don't update the array if the current frame is the same is previous one.
	if (lastFrameStamp == frameStamp) return false;
    
    int pixelSize = arUtilGetPixelSize(pixelFormat);
    switch (pixelFormat) {
        case AR_PIXEL_FORMAT_BGRA:
        case AR_PIXEL_FORMAT_BGR:
            for (int y = 0; y < videoHeight; y++) {
                ARUint8 *inp = &frameBuffer[videoWidth*y*pixelSize];
                Color *outp = &buffer[videoWidth*y];
                for (int pixelsToGo = videoWidth; pixelsToGo > 0; pixelsToGo--) {
                    outp->b = (float)*(inp + 0) / 255.0f;
                    outp->g = (float)*(inp + 1) / 255.0f;
                    outp->r = (float)*(inp + 2) / 255.0f;
                    outp->a = 1.0f ;
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_RGBA:
        case AR_PIXEL_FORMAT_RGB:
            for (int y = 0; y < videoHeight; y++) {
                ARUint8 *inp = &frameBuffer[videoWidth*y*pixelSize];
                Color *outp = &buffer[videoWidth*y];
                for (int pixelsToGo = videoWidth; pixelsToGo > 0; pixelsToGo--) {
                    outp->r = (float)*(inp + 0) / 255.0f;
                    outp->g = (float)*(inp + 1) / 255.0f;
                    outp->b = (float)*(inp + 2) / 255.0f;
                    outp->a = 1.0f;
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_ARGB:
            for (int y = 0; y < videoHeight; y++) {
                ARUint8 *inp = &frameBuffer[videoWidth*y*pixelSize];
                Color *outp = &buffer[videoWidth*y];
                for (int pixelsToGo = videoWidth; pixelsToGo > 0; pixelsToGo--) {
                    outp->r = (float)*(inp + 1) / 255.0f;
                    outp->g = (float)*(inp + 2) / 255.0f;
                    outp->b = (float)*(inp + 3) / 255.0f;
                    outp->a = 1.0f;
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_ABGR:
            for (int y = 0; y < videoHeight; y++) {
                ARUint8 *inp = &frameBuffer[videoWidth*y*pixelSize];
                Color *outp = &buffer[videoWidth*y];
                for (int pixelsToGo = videoWidth; pixelsToGo > 0; pixelsToGo--) {
                    outp->b = (float)*(inp + 1) / 255.0f;
                    outp->g = (float)*(inp + 2) / 255.0f;
                    outp->r = (float)*(inp + 3) / 255.0f;
                    outp->a = 1.0f;
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_MONO:
            for (int y = 0; y < videoHeight; y++) {
                ARUint8 *inp = &frameBuffer[videoWidth*y*pixelSize];
                Color *outp = &buffer[videoWidth*y];
                for (int pixelsToGo = videoWidth; pixelsToGo > 0; pixelsToGo--) {
                    outp->b = outp->g = outp->r = (float)*inp / 255.0f;
                    outp->a = 1.0f;
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        default:
            return false;
            break;
    }
    
    lastFrameStamp = frameStamp; // Record the new framestamp
    return true;

}

bool VideoSource::updateTexture32(uint32_t *buffer) {
    
    static int lastFrameStamp = 0;
    
    if (!buffer) return false; // Sanity check.
    
    if (!frameBuffer) return false; // Check that a frame is actually available.
    
    // Extra check: don't update the array if the current frame is the same is previous one.
    if (lastFrameStamp == frameStamp) return false;
    
    int pixelSize = arUtilGetPixelSize(pixelFormat);
    switch (pixelFormat) {
        case AR_PIXEL_FORMAT_BGRA:
        case AR_PIXEL_FORMAT_BGR:
            for (int y = 0; y < videoHeight; y++) {
                ARUint8 *inp = &frameBuffer[videoWidth*y*pixelSize];
                uint32_t *outp = &buffer[videoWidth*y];
                for (int pixelsToGo = videoWidth; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 0) << 16) | (*(inp + 1) << 8) | (*(inp + 2));
#else
                    *outp = (*(inp + 2) << 24) | (*(inp + 1) << 16) | (*(inp + 0) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_RGBA:
        case AR_PIXEL_FORMAT_RGB:
            for (int y = 0; y < videoHeight; y++) {
                ARUint8 *inp = &frameBuffer[videoWidth*y*pixelSize];
                uint32_t *outp = &buffer[videoWidth*y];
                for (int pixelsToGo = videoWidth; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 2) << 16) | (*(inp + 1) << 8) | (*(inp + 0));
#else
                    *outp = (*(inp + 0) << 24) | (*(inp + 1) << 16) | (*(inp + 2) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_ARGB:
            for (int y = 0; y < videoHeight; y++) {
                ARUint8 *inp = &frameBuffer[videoWidth*y*pixelSize];
                uint32_t *outp = &buffer[videoWidth*y];
                for (int pixelsToGo = videoWidth; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 3) << 16) | (*(inp + 2) << 8) | (*(inp + 1));
#else
                    *outp = (*(inp + 1) << 24) | (*(inp + 2) << 16) | (*(inp + 3) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_ABGR:
            for (int y = 0; y < videoHeight; y++) {
                ARUint8 *inp = &frameBuffer[videoWidth*y*pixelSize];
                uint32_t *outp = &buffer[videoWidth*y];
                for (int pixelsToGo = videoWidth; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*(inp + 1) << 16) | (*(inp + 2) << 8) | (*(inp + 3));
#else
                    *outp = (*(inp + 3) << 24) | (*(inp + 2) << 16) | (*(inp + 1) << 8) | 0xff;
#endif
                    inp += pixelSize;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_MONO:
            for (int y = 0; y < videoHeight; y++) {
                ARUint8 *inp = &frameBuffer[videoWidth*y];
                uint32_t *outp = &buffer[videoWidth*y];
                for (int pixelsToGo = videoWidth; pixelsToGo > 0; pixelsToGo--) {
#ifdef AR_LITTLE_ENDIAN
                    *outp = 0xff000000 | (*inp << 16) | (*inp << 8) | *inp;
#else
                    *outp = (*inp << 24) | (*inp << 16) | (*inp << 8) | 0xff;
#endif
                    inp++;
                    outp++;
                }
            }
            break;
        case AR_PIXEL_FORMAT_420f:
            {
                uint32_t *outp = buffer;
                float Cb, Cr;
                int Y0, Y1, R, G, B;
                ARUint8 *pY = frameBuffer;
                int wd2 = videoWidth >> 1;
                for (int y = 0; y < videoHeight; y++) {
                    ARUint8 *pCbCr = frameBuffer2 + videoWidth*(y >> 1);
                    for (int x = 0; x < wd2; x++) { // Groups of two pixels.
                        uint8_t R0, R1, G0, G1, B0, B1;
                        Y0 = *(pY++);
                        Y1 = *(pY++);
                        Cb = (float)(*(pCbCr++) - 128);
                        Cr = (float)(*(pCbCr++) - 128);
                        R = (int)((             1.402f*Cr));
                        G = (int)((-0.344f*Cb - 0.714f*Cr));
                        B = (int)(( 1.772f*Cb));
                        R0 = (uint8_t)CLAMP(Y0 + R, 0, 255);
                        R1 = (uint8_t)CLAMP(Y1 + R, 0, 255);
                        G0 = (uint8_t)CLAMP(Y0 + G, 0, 255);
                        G1 = (uint8_t)CLAMP(Y1 + G, 0, 255);
                        B0 = (uint8_t)CLAMP(Y0 + B, 0, 255);
                        B1 = (uint8_t)CLAMP(Y1 + B, 0, 255);
#ifdef AR_LITTLE_ENDIAN
                        *(outp++) = 0xff000000 | (B0 << 16) | (G0 << 8) | R0;
                        *(outp++) = 0xff000000 | (B1 << 16) | (G1 << 8) | R1;
#else
                        *(outp++) = (R0 << 24) | (G0 << 16) | (B0 << 8) | 0xff;
                        *(outp++) = (R1 << 24) | (G1 << 16) | (B1 << 8) | 0xff;
#endif
                    }
                }
            }
            break;
        case AR_PIXEL_FORMAT_NV21:
            color_convert_common((unsigned char *)frameBuffer, (unsigned char *)frameBuffer2, videoWidth, videoHeight, (unsigned char *)buffer);
            break;
        default:
            return false;
            break;
    }
    
    lastFrameStamp = frameStamp; // Record the new framestamp
    return true;
    
}

#ifndef _WINRT
void VideoSource::updateTextureGL(int textureID) {

	static int lastFrameStamp = 0;

	// Don't update the array if the current frame is the same as previous one.
	if (lastFrameStamp == frameStamp) return;

	// Record the new framestamp
	lastFrameStamp = frameStamp;

	if (textureID && frameBuffer) { // Could also chcek glIsTexture(textureID), but it is slow.
		
		//int val;
		//glGetIntegerv(GL_TEXTURE_BINDING_2D, &val);
		glBindTexture(GL_TEXTURE_2D, textureID);
#if TARGET_PLATFORM_IOS
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, glPixIntFormat, videoWidth, videoHeight, 0, glPixFormat, glPixType, frameBuffer);
#else
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, videoWidth, videoHeight, glPixFormat, glPixType, frameBuffer);
#endif
		//glBindTexture(GL_TEXTURE_2D, val);

	}
}
#endif // !_WINRT