/*
 *  videoWinDSVL.cpp
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
 *  Copyright 2004-2015 ARToolworks, Inc.
 *
 *  Author(s): Thomas Pintaric, Philip Lamb
 *
 *	Rev		Date		Who		Changes
 *	2.68.2	2004-07-20	PRL		Rewrite for ARToolKit 2.68.2
 *	2.71.0	2005-08-05	PRL		Incorporate DSVL-0.0.8b
 *
 */
/*
	========================================================================
	PROJECT: DirectShow Video Processing Library
	Version: 0.0.8 (05/04/2005)
	========================================================================
	Author:  Thomas Pintaric, Vienna University of Technology
	Contact: pintaric@ims.tuwien.ac.at http://ims.tuwien.ac.at/~thomas
	=======================================================================
	
	Copyright (C) 2005  Vienna University of Technology
	
	For further information please contact Thomas Pintaric under
	<pintaric@ims.tuwien.ac.at> or write to Thomas Pintaric,
	Vienna University of Technology, Favoritenstr. 9-11/E188/2, A-1040
	Vienna, Austria.
	========================================================================
 */

#include <AR/video.h>

#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB

#ifdef _DEBUG
#pragma comment(lib,"DSVLd.lib")
#else
#pragma comment(lib,"DSVL.lib")
#endif

#include "DSVL.h"
#include <stdlib.h>
#include "comutil.h"

// -----------------------------------------------------------------------------------------------------------------

struct _AR2VideoParamWinDSVLT {
	DSVL_VideoSource	*graphManager;
	MemoryBufferHandle  g_Handle;
	__int64				g_Timestamp; // deprecated, use (g_Handle.t) instead.
	//bool flip_horizontal = false; // deprecated.
	//bool flip_vertical = false;   // deprecated.
	AR2VideoBufferT		vidBuff;
};

// -----------------------------------------------------------------------------------------------------------------

#ifdef FLIPPED // compatibility with videoLinux*
static const bool		FLIPPED_defined =  true;		// deprecated
#else
static const bool		FLIPPED_defined =  false;		// deprecated
#endif
const long				frame_timeout_ms = 0L;	// set to INFINITE if arVideoGetImage()
														// is called from a separate worker thread

// -----------------------------------------------------------------------------------------------------------------


int ar2VideoDispOptionWinDSVL(void)
{
	ARLOG("parameter is a file name (e.g. 'config.XML') conforming to the DSVideoLib XML Schema (DsVideoLib.xsd).\n");
    return (0);
}

AR2VideoParamWinDSVLT *ar2VideoOpenWinDSVL(const char *config)
{
	AR2VideoParamWinDSVLT *vid = NULL;
	const char config_default[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><dsvl_input><camera show_format_dialog=\"true\" friendly_name=\"\"><pixel_format><RGB32 flip_h=\"false\" flip_v=\"true\"/></pixel_format></camera></dsvl_input>";
	char *config0;

	// Allocate the parameters structure and fill it in.
	arMallocClear(vid, AR2VideoParamWinDSVLT, 1);
	if (!config || !config[0]) {
		config0 = _strdup(config_default);
	} else {
		config0 = _strdup(config);
	}

	CoInitialize(NULL);
	vid->graphManager = new DSVL_VideoSource();

	if (strncmp(config0, "<?xml", 5) == 0) {
		if (FAILED(vid->graphManager->BuildGraphFromXMLString(config0))) goto bail;
	} else {
		if (FAILED(vid->graphManager->BuildGraphFromXMLFile(config0))) goto bail;
	}
	if (FAILED(vid->graphManager->EnableMemoryBuffer())) goto bail;

	free(config0);
	return (vid);

bail:
	delete vid->graphManager;
	free(config0);
	free(vid);
	return (NULL);
}


int ar2VideoCloseWinDSVL(AR2VideoParamWinDSVLT *vid)
{
	if (vid == NULL) return (-1);
	if (vid->graphManager == NULL) return (-1);
	
	vid->graphManager->CheckinMemoryBuffer(vid->g_Handle, true);
	vid->graphManager->Stop();
	delete vid->graphManager;
	vid->graphManager = NULL;
	free (vid);
	
    return(0);
}

AR2VideoBufferT *ar2VideoGetImageWinDSVL(AR2VideoParamWinDSVLT *vid)
{
	DWORD wait_result;
	
	if (vid == NULL) return (NULL);
	if (vid->graphManager == NULL) return (NULL);
	
	// Ideally, we'd check this in earlier.
	if (vid->vidBuff.fillFlag) {
		if (FAILED(vid->graphManager->CheckinMemoryBuffer(vid->g_Handle)) ) return (NULL);
	}
	vid->vidBuff.fillFlag = 0;
	
	wait_result = vid->graphManager->WaitForNextSample(INFINITE);
	if (wait_result == WAIT_OBJECT_0) {
		if (SUCCEEDED(vid->graphManager->CheckoutMemoryBuffer(&(vid->g_Handle), &(vid->vidBuff.buff), NULL, NULL, NULL, &(vid->g_Timestamp)))) {
			vid->vidBuff.fillFlag = 1;
			return (&(vid->vidBuff));
		}
	}

	return(NULL);
}

int ar2VideoCapStartWinDSVL(AR2VideoParamWinDSVLT *vid)
{
	if (vid == NULL) return (-1);
	if (vid->graphManager == NULL) return (-1);
	
	HRESULT result = vid->graphManager->Run();
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES)) return -2;
		else return -1;
	}
	return (0);
}

int ar2VideoCapStopWinDSVL(AR2VideoParamWinDSVLT *vid)
{
	if (vid == NULL) return (-1);
	if (vid->graphManager == NULL) return (-1);

	vid->graphManager->CheckinMemoryBuffer(vid->g_Handle, true);

	// PRL 2005-09-21: Commented out due to issue where stopping the
	// media stream cuts off glut's periodic tasks, including functions
	// registered with glutIdleFunc() and glutDisplayFunc();
	//if(FAILED(vid->graphManager->Stop())) return (-1);

	return (0);
}

// -----------------------------------------------------------------------------------------------------------------

#if 0
int ar2VideoInqFlipping(AR2VideoParamT *vid, int *flipH, int *flipV)
{
	// DEPRECATED
	// image flipping can be specified in the XML config file, but can

	// no longer be queried via arVideoInqFlipping()

	return (-1); // not implemented
}

int ar2VideoInqFreq(AR2VideoParamT *vid, float *fps)
{
	if (vid == NULL) return (-1);
	if (vid->graphManager == NULL) return(-1);

	double frames_per_second;

	vid->graphManager->GetCurrentMediaFormat(NULL,NULL,&frames_per_second,NULL);

	*fps = (float) frames_per_second;


    return (0);
}

unsigned char *ar2VideoLockBuffer(AR2VideoParamT *vid, MemoryBufferHandle* pHandle)
{
	unsigned char *pixelBuffer;
	
	if (vid == NULL) return (NULL);
	if (vid->graphManager == NULL) return (NULL);
	
	if (FAILED(vid->graphManager->CheckoutMemoryBuffer(pHandle, &pixelBuffer))) return (NULL);
	
	return (pixelBuffer);
}

int ar2VideoUnlockBuffer(AR2VideoParamT *vid, MemoryBufferHandle Handle)
{
	if (vid == NULL) return (-1);
	if (vid->graphManager == NULL) return(-1);
	
	if (FAILED(vid->graphManager->CheckinMemoryBuffer(Handle))) return(-1);
	
	return (0);
}
#endif

// -----------------------------------------------------------------------------------------------------------------

int ar2VideoGetSizeWinDSVL(AR2VideoParamWinDSVLT *vid, int *x, int *y)
{
	if (vid == NULL) return (-1);
	if (vid->graphManager == NULL) return(-1);
	
	long frame_width;
	long frame_height;

	vid->graphManager->GetCurrentMediaFormat(&frame_width, &frame_height,NULL,NULL);
	*x = (int) frame_width;
	*y = (int) frame_height;
	
    return (0);
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatWinDSVL( AR2VideoParamWinDSVLT *wvid )
{
    return AR_INPUT_WINDOWS_DSVIDEOLIB_PIXEL_FORMAT;
}

int ar2VideoGetIdWinDSVL( AR2VideoParamWinDSVLT *wvid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiWinDSVL( AR2VideoParamWinDSVLT *wvid, int paramName, int *value )
{
    return -1;
}

int ar2VideoSetParamiWinDSVL( AR2VideoParamWinDSVLT *wvid, int paramName, int  value )
{
    return -1;
}

int ar2VideoGetParamdWinDSVL( AR2VideoParamWinDSVLT *wvid, int paramName, double *value )
{
    return -1;
}

int ar2VideoSetParamdWinDSVL( AR2VideoParamWinDSVLT *wvid, int paramName, double  value )
{
    return -1;
}

#endif // AR_INPUT_WINDOWS_DSVIDEOLIB