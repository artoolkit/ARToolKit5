/*
 *  videoWinDSPrivate.cpp
 *  ARToolKit5
 *
 *  DirectShow video capture module.
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
 *  Copyright 2005-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.es.osaka-u.ac.jp
 *
 * Revision: 4.1
 * Date: 2005/08/26
 *
 *******************************************************/

#ifndef AR_VIDEO_WIN_DS_PRIVATE_H
#define AR_VIDEO_WIN_DS_PRIVATE_H

#include <windows.h>
#include <dshow.h>
#include <qedit.h>
#include <stdio.h>
#include <AR/video.h>

#define		AR2VIDEO_WINDS_DEVICE_MAX			10


#define		AR2VIDEO_WINDS_STATUS_IDLE			 0
#define		AR2VIDEO_WINDS_STATUS_RUN			 1


typedef struct {
	IGraphBuilder			*pGraph;
	ICreateDevEnum			*pDevEnum;
	IEnumMoniker			*pClassEnum;
	IMediaControl			*pMediaControl;
	int						 devNum;
	char					*devName[AR2VIDEO_WINDS_DEVICE_MAX];
	int						 devStatus[AR2VIDEO_WINDS_DEVICE_MAX];
	int						 runCount;
} AR2VideoWinDSDeviceInfoT;



typedef struct {
    AR2VideoBufferT			 in;
    AR2VideoBufferT		 	 wait;
    AR2VideoBufferT			 out;
	HANDLE					 buffMutex;
	void					*pBmpBuffer;
	long					 bmpBufferSize;
	int						 width;
	int						 height;
	int						 flipH;
	int						 flipV;
	int                      status;
} AR2VideoBufferWinDST;

class ARSampleGrabberCB : public ISampleGrabberCB
{
public:
    AR2VideoBufferWinDST     buffer;
	ISampleGrabber			*pGrabber;

	ARSampleGrabberCB(int flipH, int flipV);

    STDMETHODIMP_(ULONG) AddRef() { return 1; }
    STDMETHODIMP_(ULONG) Release() { return 2; }

    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP SampleCB(double Time, IMediaSample *pSample);
    STDMETHODIMP BufferCB(double Time, BYTE *pBuffer, long BufferLen);
};

typedef struct {
	IBaseFilter				*pVideoCapFilter;
	IBaseFilter				*pVideoGrabFilter;
	ARSampleGrabberCB		*grabberCallback;
	int						 showPropertiesFlag;
	int						 devNum;
} AR2VideoParamWinDS2T;

#endif
