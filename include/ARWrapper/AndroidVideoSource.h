/*
 *  AndroidVideoSource.h
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

#ifndef ANDROIDVIDEOSOURCE_H
#define ANDROIDVIDEOSOURCE_H

#include <ARWrapper/Platform.h>
#include <ARWrapper/VideoSource.h>

#if TARGET_PLATFORM_ANDROID

/**
 * Video input implementation for Android. On Android, video capture occurs in Java, and the frame data 
 * is passed across to the native code using JNI. Therefore, ARToolKit cannot open the camera and 
 * initiate video capture in the same way as it does on other platforms. Instead, the video source remains 
 * closed until the first frame arrives over JNI.
 */
class AndroidVideoSource : public VideoSource {

private:

	bool newFrameArrived;
    ARUint8 *localFrameBuffer;
    size_t frameBufferSize;

    static void getVideoReadyAndroidCparamCallback(const ARParam *cparam_p, void *userdata);
    bool getVideoReadyAndroid2(const ARParam *cparam_p);

protected:
    
    AR2VideoParamT *gVid;
    int gCameraIndex;
    bool gCameraIsFrontFacing;

public:

	AndroidVideoSource();

	bool getVideoReadyAndroid(const int width, const int height, const int cameraIndex, const bool cameraIsFrontFacing);

	virtual bool open();

    /**
     * Returns the size of current frame.
     * @return		Size of the buffer containing the current video frame
     */
    size_t getFrameSize();
    
	void acceptImage(ARUint8* ptr);

	virtual bool captureFrame();

	virtual bool close();

	virtual const char* getName();

};

#endif

#endif // !ANDROIDVIDEOSOURCE_H
