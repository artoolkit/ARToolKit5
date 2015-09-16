/*
 *  VideoSource.h
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
 *  Author(s): Julian Looser, Philip Lamb
 *
 */

#ifndef VIDEOSOURCE_H
#define VIDEOSOURCE_H

#include "Platform.h"

#include <AR/ar.h>
#include <AR/video.h>

#include <ARWrapper/Image.h>

#ifndef _WINRT
#  if TARGET_PLATFORM_ANDROID || TARGET_PLATFORM_IOS
#    include <AR/gsub_es.h>
#  else
#    include <AR/gsub_lite.h>
  #endif
#endif

/**
 * Base class for different video source implementations. A video source provides video frames to the 
 * ARToolKit tracking module. Video sources contain information about the video, such as size and pixel 
 * format, camera parameters for distortion compensation, as well as the raw video data itself. Different 
 * video source implementations are required to provide a standard interface for video input, even though 
 * different platforms handle video input quite differently.
 */
class VideoSource {

protected:

	typedef enum {
		DEVICE_CLOSED,					///< Device is closed.
		DEVICE_OPEN,					///< Device is open.
        DEVICE_GETTING_READY,           ///< Device is moving from open to running.
		DEVICE_RUNNING					///< Device is open and able to be queried.
	} DeviceState;
    
	DeviceState deviceState;			///< Current state of the video device

	char* cameraParam;					///< Camera parameter filename
	char* cameraParamBuffer;
	size_t cameraParamBufferLen;
	ARParamLT *cparamLT;				///< Camera paramaters

	char* videoConfiguration;			///< Video configuration string

	int videoWidth;						///< Width of the video frame in pixels
	int videoHeight;					///< Height of the video frame in pixels

	AR_PIXEL_FORMAT pixelFormat;		///< Pixel format from ARToolKit enumeration.
#if defined(HAVE_ARM_NEON) || defined(HAVE_ARM64_NEON)
    int m_fastPath;
#endif
#ifndef _WINRT
	GLenum glPixIntFormat;
    GLenum glPixFormat;
    GLenum glPixType;
#endif

    ARUint8 *frameBuffer;               ///< Pointer to latest frame. Set by concrete subclass to point to frame data.
    ARUint8 *frameBuffer2;              ///< For bi-planar formats, pointer to plane 2 of latest frame. Set by concrete subclass to point to frame data.
	int frameStamp;						///< Latest framestamp. Incremented in the concrete subclass when a new frame arrives.
    
    int m_error;
    void setError(int error);

	/**
	 * The constructor is not public because instances are created using a factory method.
	 * @see newVideoSource()
	 */
	VideoSource();
	
public:

	/**
	 * Returns the correct VideoSource subclass for use on the current platform.
	 * @return New instance of an appropriate VideoSource
	 */
	static VideoSource* newVideoSource();
    
	virtual ~VideoSource();

    int getError();

    /**
	 * Returns true if the video source is open
	 * @return	true if the video source is open
	 */
	bool isOpen();

	/**
	 * Returns true if the video source is open and ready to be queried.
	 * @return	true if the video source is open and frame details are known
	 */
	bool isRunning();
    
	/**
	 * Sets initial parameters which will be used when the video source
     * is opened.
     */
	void configure(const char* vconf, const char* cparaName, const char* cparaBuff, size_t cparaBuffLen);


	/**
	 * Returns the camera parameters for the video source.
	 * @return  the camera parameters
	 */
	ARParamLT* getCameraParameters();

	/**
	 * Returns the width of the video in pixels.
	 * @return		Width of the video in pixels
	 */
	int getVideoWidth();
	
	/**
	 * Returns the height of the video in pixels.
	 * @return		Height of the video in pixels
	 */
	int getVideoHeight();

	/**
	 * Returns the pixel format of the video.
	 * @return		Pixel format of the video
	 */
	AR_PIXEL_FORMAT getPixelFormat();

	/**
	 * Returns the name of this video source variation. This method must be provided by 
	 * the subclass.
	 * @return		Name of the video source
	 */
	virtual const char* getName() = 0;
	
	/**
	 * Opens the video source. This method must be provided by the subclass.
	 * @return		true if the video source was opened successfully, false if a fatal error occured.
	 */
	virtual bool open() = 0;
	
	/**
	 * Closes the video source. This method must be provided by the subclass.
	 * @return		true if the video source was closed successfully, otherwise false.
	 */
	virtual bool close() = 0;

	/**
	 * Asks the video source to capture a frame. This method must be provided by the subclass.
	 * @return		true if the video source captured a frame, otherwise false
	 */
	virtual bool captureFrame() = 0;

	/**
	 * Returns the current frame.
	 * @return		Pointer to the buffer containing the current video frame
	 */
	ARUint8* getFrame();

	/**
	 * Returns the current frame stamp. If the returned value has changed since the last 
	 * time this function was called, then the caller can assume a new frame is available.
	 * @return		The current frame stamp, incremented on each new frame arrival
	 */
	int getFrameStamp();

	/**
	 * Populates the provided color buffer with the current video frame.
	 * @param buffer	The color buffer to populate with frame data
	 * @return			true if the buffer was updated successfully, otherwise false
	 */
	bool updateTexture(Color* buffer);

    bool fastPath();
    bool updateTexture32(uint32_t *buffer);
    
#ifndef _WINRT
	/**
	 * Updates the specified OpenGL texture with the current video frame
	 * @param textureID	The OpenGL texture ID to which the video frame should be uploaded
	 */
	void updateTextureGL(int textureID);
#endif
};

#endif // !VIDEOSOURCE_H
