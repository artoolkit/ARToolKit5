/*
 *  videoWinDS.cpp
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

#include <AR/video.h>

#ifdef AR_INPUT_WINDOWS_DIRECTSHOW

#pragma comment(lib,"strmiids.lib")

#include <windows.h>
#include <iostream>
#pragma include_alias( "dxtrans.h", "qedit.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
#include <qedit.h>
#include <stdio.h>
#include <atlstr.h>
#include <dshow.h>
#include <sys/timeb.h>
#include <videoWinDSPrivate.h>
#include <comdef.h>

static AR2VideoWinDSDeviceInfoT	*ar2VideoWinDSInit2( void );
static int                       ar2VideoWinDSFinal2(AR2VideoWinDSDeviceInfoT	**devInfo_p);
static IPin						*ar2VideoWinDSGetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir);
static HRESULT					 ar2VideoWinDSDisplayFilterProperties( IBaseFilter *pFilter );
static HRESULT					 ar2VideoWinDSDisplayPinProperties(CComPtr<IPin> pSrcPin);
static void						 ar2VideoWinDSGetTimeStamp(ARUint32 *t_sec, ARUint32 *t_usec);
static void						 ar2VideoWinDSFlipImageRGB24(ARUint8 *iBuf, ARUint8 *oBuf, int width, int height, int flipH, int flipV);
static int						 ar2VideoWinDSShowProperties( AR2VideoParamWinDST *wvid );
static void						 ar2VideoWinDSShowProperties2( void *wvid );



static AR2VideoWinDSDeviceInfoT	*ar2VideoWinDSDeviceInfo = NULL;
static int ar2VideoWinDSDeviceInfoRefCount = 0;



int ar2VideoDispOptionWinDS( void )
{
    ARLOG(" -device=WinDS\n");
    ARLOG(" -showDeviceList\n");
    ARLOG(" -showDialog\n");
	ARLOG(" -devNum=N\n");
	ARLOG(" -flipH\n");
	ARLOG(" -flipV\n");
    ARLOG("\n");

    return 0;
}

ARVideoSourceInfoListT *ar2VideoCreateSourceInfoListWinDS(const char *config)
{
	int i;
	ARVideoSourceInfoListT *list = NULL;

	if (ar2VideoWinDSDeviceInfo == NULL) {
		if ((ar2VideoWinDSDeviceInfo = ar2VideoWinDSInit2()) == NULL) return NULL;
	}
	ar2VideoWinDSDeviceInfoRefCount++;

	// Fill the list.
	arMallocClear(list, ARVideoSourceInfoListT, 1);
	list->count = ar2VideoWinDSDeviceInfo->devNum;
	arMallocClear(list->info, ARVideoSourceInfoT, ar2VideoWinDSDeviceInfo->devNum);
	for (i = 0; i < ar2VideoWinDSDeviceInfo->devNum; i++) {
		list->info[i].name = strdup(ar2VideoWinDSDeviceInfo->devName[i]);
		if (ar2VideoWinDSDeviceInfo->devStatus[i] == 1) list->info[i].flags |= AR_VIDEO_SOURCE_INFO_FLAG_IN_USE;
	}

//bail:
	ar2VideoWinDSDeviceInfoRefCount--;
	if (ar2VideoWinDSDeviceInfoRefCount == 0) ar2VideoWinDSFinal2(&ar2VideoWinDSDeviceInfo);

	return list;
}

AR2VideoParamWinDST *ar2VideoOpenWinDS( const char *config )
{
    AR2VideoParamWinDS2T	*vid;
	IMoniker				*pMoniker;
	ULONG					 cFetched;
	AM_MEDIA_TYPE			 mt;
	HRESULT					 hr;
	int						 devNum = -1;
	int						 showList = -1;
	int						 showDialog = -1;
	int						 flipH = -1;
	int						 flipV = -1;
	const char				*a;
    char                     b[256];
	const char config_default[] = "-showDialog -flipV";

	//ARLOG("Entering ar2VideoOpenWinDS\n");
	if( ar2VideoWinDSDeviceInfo == NULL ) {
		if( (ar2VideoWinDSDeviceInfo = ar2VideoWinDSInit2()) == NULL ) return NULL;
	}
    ar2VideoWinDSDeviceInfoRefCount++;

	// Ensure the provided config is valid, otherwise use default config.
	if (!config) a = config_default;
	else if (!config[0]) a = config_default;
    else a = config;
    if( a != NULL) {
        for(;;) {
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\0' ) break;
    
            if( sscanf(a, "%s", b) == 0 ) break;
            if( strncmp( b, "-devNum=", 8 ) == 0 ) {
                if( sscanf( &b[8], "%d", &devNum ) != 1 ) {
                    ar2VideoDispOptionWinDS();
                    return NULL;
                }
				else if( devNum < 1 || devNum > ar2VideoWinDSDeviceInfo->devNum ) {
					ARLOGe("error: illegal device number.\n");
					return NULL;
				}
				else if( ar2VideoWinDSDeviceInfo->devStatus[devNum-1] == 1 ) {
					ARLOGe("error: device already in use.\n");
					return NULL;
				}
            }
            else if( strcmp( b, "-showDeviceList") == 0 ) {
				showList = 1;
            }
            else if( strcmp( b, "-showDialog" ) == 0 )    {
				showDialog = 1;
            }
            else if( strcmp( b, "-flipH" ) == 0 )    {
				flipH = 1;
            }
            else if( strcmp( b, "-flipV" ) == 0 )    {
				flipV = 1;
            }
			else if ( strcmp( b, "-device=WinDS") == 0) {
				//ARLOG("Device set to WinDS\n");
			}
            else {
				ARLOGe("Unrecognized config token: '%s'\n", b);
                ar2VideoDispOptionWinDS();
                return 0;
            }

            while( *a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

	if( showList == 1 ) {
		for( int i = 0; i < ar2VideoWinDSDeviceInfo->devNum; i++ ) {
			ARLOG("%2d: \"%s\"\n", i+1, ar2VideoWinDSDeviceInfo->devName[i]);
		}
        goto bail;
	}

	if( devNum == -1 ) {
		int  i;
		for( i = 0; i < ar2VideoWinDSDeviceInfo->devNum; i++ ) {
			if( ar2VideoWinDSDeviceInfo->devStatus[i] == 0 ) break;
		}
		if( i == ar2VideoWinDSDeviceInfo->devNum ) {
			ARLOGe("error: no available devices.\n");
			goto bail;
		}
		devNum = i+1;
	}

	ar2VideoWinDSDeviceInfo->devStatus[devNum-1] = 1;

	arMalloc(vid, AR2VideoParamWinDS2T, 1);
	vid->grabberCallback = new ARSampleGrabberCB(flipH, flipV);
	vid->devNum = devNum;

	hr = ar2VideoWinDSDeviceInfo->pClassEnum->Reset(); // Reset the enumeration to the beginning.
	if( FAILED(hr) ) {
		ARLOGe("Error!! pClassEnum->Reset\n");
		exit(0);
	}

	// Skip over the required number of devices to reach the specified item in the enumeration
	if( devNum > 1 ) {
		hr = ar2VideoWinDSDeviceInfo->pClassEnum->Skip(devNum - 1);
		if( FAILED(hr) ) {
			ARLOGe("Error!! pClassEnum->Skip\n");
			exit(0);
		}
	}


	// Retrieve the item from the enumeration
	hr = ar2VideoWinDSDeviceInfo->pClassEnum->Next(1, &pMoniker, &cFetched);
	if( FAILED(hr) ) {
		ARLOGe("Error!! pClassEnum->Next\n");
		exit(0);
	}
	pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&(vid->pVideoCapFilter));
	pMoniker->Release();
	hr = ar2VideoWinDSDeviceInfo->pGraph->AddFilter(vid->pVideoCapFilter, L"Video Capture");
	if( (FAILED(hr)) ) {
		ARLOGe("Error AddFilter:pVideoCapFilter\n");
		ARLOGe(_com_error(hr).ErrorMessage());
		exit(0);
	}

	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_ISampleGrabber, (void**)&(vid->grabberCallback->pGrabber));
	if( (FAILED(hr)) ) {
		ARLOGe("Error CoCreateInstance\n");
		exit(0);
	}
	hr = vid->grabberCallback->pGrabber->QueryInterface(IID_IBaseFilter, (void**)&(vid->pVideoGrabFilter));
	if( (FAILED(hr)) ) {
		ARLOGe("Error QueryInterface\n");
		exit(0);
	}
	hr = ar2VideoWinDSDeviceInfo->pGraph->AddFilter(vid->pVideoGrabFilter, L"Grabber");
	if( (FAILED(hr)) ) {
		ARLOGe("Error AddFilter:pVideoGrabFilter\n");
		exit(0);
	}


	ZeroMemory(&mt, sizeof(mt));
	mt.majortype  = MEDIATYPE_Video;
	mt.subtype    = MEDIASUBTYPE_RGB24;
	mt.formattype = FORMAT_VideoInfo;
	hr = vid->grabberCallback->pGrabber->SetMediaType(&mt);
	if( (FAILED(hr)) ) {
		ARLOGe("Error SetMediaType\n");
		exit(0);
	}
	hr = vid->grabberCallback->pGrabber->SetCallback( vid->grabberCallback, 0 );
	if( (FAILED(hr)) ) {
		ARLOGe("Error SetCallback\n");
		exit(0);
	}


	IPin *pSrcOut  = ar2VideoWinDSGetPin(vid->pVideoCapFilter, PINDIR_OUTPUT);
	IPin *pSGrabIN = ar2VideoWinDSGetPin(vid->pVideoGrabFilter, PINDIR_INPUT);
	if( showDialog == 1 ) ar2VideoWinDSDisplayPinProperties(pSrcOut);
	hr = ar2VideoWinDSDeviceInfo->pGraph->Connect(pSrcOut, pSGrabIN);
	if( FAILED(hr) ) {
		ARLOGe("pGraph->Connect\n");
		exit(0);
	}
	hr = vid->grabberCallback->pGrabber->GetConnectedMediaType(&mt);
	if( (FAILED(hr)) ) {
		ARLOGe("Error GetConnectedMediaType\n");
		if( (hr) == VFW_E_NOT_CONNECTED ) ARLOGe("VFW_E_NOT_CONNECTED\n");
		if( (hr) == E_POINTER ) ARLOGe("E_POINTER\n");
		exit(0);
	}

	VIDEOINFOHEADER	*pVideoHeader;
	BITMAPINFO		 bminfo;

	pVideoHeader = (VIDEOINFOHEADER*)mt.pbFormat;
	ZeroMemory(&bminfo, sizeof(bminfo));
	CopyMemory(&bminfo.bmiHeader, &(pVideoHeader->bmiHeader), sizeof(BITMAPINFOHEADER));

	CreateDIBSection(0, &bminfo, DIB_RGB_COLORS, &(vid->grabberCallback->buffer.pBmpBuffer), NULL, 0);
	CreateDIBSection(0, &bminfo, DIB_RGB_COLORS, (void **)(&(vid->grabberCallback->buffer.in.buff)),    NULL, 0);
	CreateDIBSection(0, &bminfo, DIB_RGB_COLORS, (void **)(&(vid->grabberCallback->buffer.out.buff)),   NULL, 0);
	CreateDIBSection(0, &bminfo, DIB_RGB_COLORS, (void **)(&(vid->grabberCallback->buffer.wait.buff)),  NULL, 0);
	vid->grabberCallback->buffer.in.fillFlag   = 0;
    vid->grabberCallback->buffer.out.fillFlag  = 0;
    vid->grabberCallback->buffer.wait.fillFlag = 0;
	vid->grabberCallback->buffer.in.buffLuma   = NULL;
    vid->grabberCallback->buffer.out.buffLuma  = NULL;
    vid->grabberCallback->buffer.wait.buffLuma = NULL;
	vid->grabberCallback->buffer.buffMutex     = CreateMutex( NULL, FALSE, NULL );
	vid->grabberCallback->buffer.bmpBufferSize = mt.lSampleSize;
	vid->grabberCallback->buffer.width         = bminfo.bmiHeader.biWidth;
	vid->grabberCallback->buffer.height        = bminfo.bmiHeader.biHeight;
	ARLOG("bmpBufferSize = %d, width = %d, height = %d\n", vid->grabberCallback->buffer.bmpBufferSize, bminfo.bmiHeader.biWidth, bminfo.bmiHeader.biHeight);

	hr = vid->grabberCallback->pGrabber->SetBufferSamples(TRUE);
	if( (FAILED(hr)) ) {
		ARLOGe("Error SetBufferSamples\n");
		exit(0);
	}

	if( showDialog == 1 ) ar2VideoWinDSDisplayFilterProperties(vid->pVideoCapFilter);
	vid->showPropertiesFlag = 0;

	return (AR2VideoParamWinDST *) vid;
    
bail:
    ar2VideoWinDSDeviceInfoRefCount--;
    if (ar2VideoWinDSDeviceInfoRefCount == 0) ar2VideoWinDSFinal2(&ar2VideoWinDSDeviceInfo);
    return NULL;
}

int ar2VideoCloseWinDS( AR2VideoParamWinDST *wvid )
{
	AR2VideoParamWinDS2T	*vid = (AR2VideoParamWinDS2T *)wvid;
	HRESULT					 hr;

	if( vid->grabberCallback->buffer.status == AR2VIDEO_WINDS_STATUS_RUN ) {
		ar2VideoCapStopWinDS( vid );
	}

	hr = vid->grabberCallback->pGrabber->SetCallback( NULL, 0 );
	if( (FAILED(hr)) ) {
		ARLOGe("Error SetCallback\n");
		exit(0);
	}

	DeleteObject(vid->grabberCallback->buffer.pBmpBuffer);
	DeleteObject(vid->grabberCallback->buffer.in.buff);
	DeleteObject(vid->grabberCallback->buffer.out.buff);
	DeleteObject(vid->grabberCallback->buffer.wait.buff);
	vid->grabberCallback->pGrabber->Release();

	CloseHandle( vid->grabberCallback->buffer.buffMutex );

	ar2VideoWinDSDeviceInfo->devStatus[vid->devNum-1] = 0;
	vid->pVideoGrabFilter->Release();
	delete vid->grabberCallback;

	free( vid );

    ar2VideoWinDSDeviceInfoRefCount--;
    if (ar2VideoWinDSDeviceInfoRefCount == 0) ar2VideoWinDSFinal2(&ar2VideoWinDSDeviceInfo);
    
    return 0;
} 

int ar2VideoCapStartWinDS( AR2VideoParamWinDST *wvid )
{
	AR2VideoParamWinDS2T	*vid = (AR2VideoParamWinDS2T *)wvid;
	HRESULT					 hr;

    if(vid->grabberCallback->buffer.status == AR2VIDEO_WINDS_STATUS_RUN){
        ARLOGe("arVideoCapStart has already been called.\n");
        return -1;
    }

	if( ar2VideoWinDSDeviceInfo->runCount == 0 ) {
		hr = ar2VideoWinDSDeviceInfo->pMediaControl->Run();
		if( (FAILED(hr)) ) {
			ARLOGe("Error %d pMediaControl->Run\n", hr);
			return -2;
		}
	}
	ar2VideoWinDSDeviceInfo->runCount++;

	vid->grabberCallback->buffer.status = AR2VIDEO_WINDS_STATUS_RUN;

    return 0;
}

int ar2VideoCapStopWinDS( AR2VideoParamWinDST *wvid )
{
	AR2VideoParamWinDS2T	*vid = (AR2VideoParamWinDS2T *)wvid;
	HRESULT					 hr;

	if( vid->grabberCallback->buffer.status == AR2VIDEO_WINDS_STATUS_IDLE ) {
        ARLOGe("arVideoCapStop has already been called.\n");
		return -1;
	}

	if( ar2VideoWinDSDeviceInfo->runCount == 1 ) {
		hr = ar2VideoWinDSDeviceInfo->pMediaControl->Stop();
		if( (FAILED(hr)) ) {
			ARLOGe("Error pMediaControl->Stop\n");
			return -1;
		}
	}
	ar2VideoWinDSDeviceInfo->runCount--;

    vid->grabberCallback->buffer.status = AR2VIDEO_WINDS_STATUS_IDLE;

    return 0;
}

AR2VideoBufferT *ar2VideoGetImageWinDS( AR2VideoParamWinDST *wvid )
{
	AR2VideoParamWinDS2T	*vid = (AR2VideoParamWinDS2T *)wvid;
    AR2VideoBufferT			 tmp;

	WaitForSingleObject( vid->grabberCallback->buffer.buffMutex, INFINITE );
	tmp = vid->grabberCallback->buffer.wait;
    if (!tmp.fillFlag) {
        ReleaseMutex( vid->grabberCallback->buffer.buffMutex );
        return (NULL);
    }
	vid->grabberCallback->buffer.wait = vid->grabberCallback->buffer.out; // Return the previous buffer to the pool.
	vid->grabberCallback->buffer.wait.fillFlag = 0;
	vid->grabberCallback->buffer.out = tmp; // Grab the new one.
	ReleaseMutex( vid->grabberCallback->buffer.buffMutex );

    return &(vid->grabberCallback->buffer.out);
}

int ar2VideoGetSizeWinDS(AR2VideoParamWinDST *wvid, int *x,int *y)
{
	AR2VideoParamWinDS2T	*vid = (AR2VideoParamWinDS2T *)wvid;

	*x = vid->grabberCallback->buffer.width;
	*y = vid->grabberCallback->buffer.height;

	return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatWinDS( AR2VideoParamWinDST *wvid )
{
    return AR_INPUT_WINDOWS_DIRECTSHOW_PIXEL_FORMAT;
}

int ar2VideoGetIdWinDS( AR2VideoParamWinDST *wvid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiWinDS( AR2VideoParamWinDST *wvid, int paramName, int *value )
{
    return -1;
}

int ar2VideoSetParamiWinDS( AR2VideoParamWinDST *wvid, int paramName, int  value )
{
	if( paramName == AR_VIDEO_WINDS_SHOW_PROPERTIES ) {
		ar2VideoWinDSShowProperties( wvid );
		return 0;
	}

    return -1;
}

int ar2VideoGetParamdWinDS( AR2VideoParamWinDST *wvid, int paramName, double *value )
{
    return -1;
}

int ar2VideoSetParamdWinDS( AR2VideoParamWinDST *wvid, int paramName, double  value )
{
    return -1;
}






static AR2VideoWinDSDeviceInfoT	*ar2VideoWinDSInit2( void )
{
	AR2VideoWinDSDeviceInfoT	*devInfo;
	IMoniker					*pMoniker;
	ULONG						 cFetched;
	HRESULT						 hr;

	//ARLOG("Entering ar2VideoWinDSInit2\n");

    arMalloc(devInfo, AR2VideoWinDSDeviceInfoT, 1);

	CoInitialize(NULL);

	// Filter graph
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&(devInfo->pGraph));
	if( FAILED(hr) ) {
		ARLOGe("Error!! CoCreateInstance: IID_IGraphBuilder\n");
		exit(0);
	}

	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void **)&(devInfo->pDevEnum));
	if( FAILED(hr) ) {
		ARLOGe("Error!! CoCreateInstance: IID_ICreateDevEnum\n");
		exit(0);
	}
	hr= devInfo->pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &(devInfo->pClassEnum), 0);
	if( FAILED(hr) ) {
		ARLOGe("Error!! CreateClassEnumerator\n");
		exit(0);
	}
    if (!devInfo->pClassEnum) {
        ARLOG("No video input devices available. Did you connect a camera?\n");
        return (NULL);
    }

	devInfo->devNum = 0;
	while (devInfo->pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
		IPropertyBag	*pPropBag;
		VARIANT			 varName;

		// Get the friendly name
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
		varName.vt = VT_BSTR;
		pPropBag->Read(L"FriendlyName", &varName, 0);

		// Allocate space for, and copy the friendly name.
		arMalloc(devInfo->devName[devInfo->devNum], char, SysStringLen(varName.bstrVal) + 1);
		WideCharToMultiByte( CP_ACP, 0, varName.bstrVal, SysStringLen(varName.bstrVal), devInfo->devName[devInfo->devNum], SysStringLen(varName.bstrVal), 0, 0);
		devInfo->devName[devInfo->devNum][SysStringLen(varName.bstrVal)] = '\0';
		devInfo->devStatus[devInfo->devNum] = 0;
		//ARLOG("Found WinDS device %d: %s\n", devInfo->devNum, devInfo->devName[devInfo->devNum]);

		SysFreeString(varName.bstrVal);
		pPropBag->Release();
		pMoniker->Release();

		(devInfo->devNum)++;
		if( devInfo->devNum == AR2VIDEO_WINDS_DEVICE_MAX ) {
			ARLOGe("Exceeded size of WinDS device list (%d)\n", AR2VIDEO_WINDS_DEVICE_MAX);
			break;
		}

	}

	hr = devInfo->pGraph->QueryInterface(IID_IMediaControl, (void **)&(devInfo->pMediaControl));
	if( (FAILED(hr)) ) {
		ARLOGe("Error QueryInterface:IID_IMediaControl\n");
		exit(0);
	}
    
	devInfo->runCount = 0;

	return devInfo;
}

// Free a devInfo struct.
static int ar2VideoWinDSFinal2(AR2VideoWinDSDeviceInfoT	**devInfo_p)
{
    int i;
    
 	(*devInfo_p)->pMediaControl->Release();
    (*devInfo_p)->pMediaControl = NULL;
    
    for (i = 0; i < (*devInfo_p)->devNum; i++) {
        free((*devInfo_p)->devName[i]);
        (*devInfo_p)->devName[i] = NULL;
    }
    (*devInfo_p)->devNum = 0;
   
	(*devInfo_p)->pClassEnum->Release();
    (*devInfo_p)->pClassEnum = NULL;
	(*devInfo_p)->pDevEnum->Release();
    (*devInfo_p)->pDevEnum = NULL;
	(*devInfo_p)->pGraph->Release();
    (*devInfo_p)->pGraph = NULL;

    CoUninitialize();
    
    free(*devInfo_p);
    *devInfo_p = NULL;

	return (0);
}


static IPin *ar2VideoWinDSGetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir)
{
	BOOL        bFound = FALSE;
	IEnumPins  *pEnum;
	IPin       *pPin;

	pFilter->EnumPins(&pEnum);
	while(pEnum->Next(1, &pPin, 0) == S_OK) {
		PIN_DIRECTION PinDirThis;
		pPin->QueryDirection(&PinDirThis);
		if (bFound = (PinDir == PinDirThis)) break;
		pPin->Release();
	}
	pEnum->Release();
	return (bFound ? pPin : 0);
}

static HRESULT ar2VideoWinDSDisplayPinProperties(CComPtr<IPin> pSrcPin)
{
	CComPtr<ISpecifyPropertyPages> pPages;

	HRESULT hr = pSrcPin->QueryInterface(IID_ISpecifyPropertyPages, (void**)&pPages);
	if (SUCCEEDED(hr)) {
		PIN_INFO PinInfo;
		pSrcPin->QueryPinInfo(&PinInfo);

		CAUUID caGUID;
		pPages->GetPages(&caGUID);

		OleCreatePropertyFrame( NULL, 0, 0, L"Property Sheet", 1, (IUnknown **)&(pSrcPin.p), caGUID.cElems, caGUID.pElems, 0, 0, NULL);
		CoTaskMemFree(caGUID.pElems);
		PinInfo.pFilter->Release();
	}
	else return(hr);

	return(S_OK);
}

static HRESULT ar2VideoWinDSDisplayFilterProperties( IBaseFilter *pFilter )
{
	HRESULT			hr;

	ISpecifyPropertyPages *pSpecify;
	hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify);
	if (SUCCEEDED(hr)) {
		FILTER_INFO FilterInfo;
		pFilter->QueryFilterInfo(&FilterInfo);

		CAUUID caGUID;
		pSpecify->GetPages(&caGUID);
		pSpecify->Release();

		OleCreatePropertyFrame( NULL, 0, 0, FilterInfo.achName, 1, (IUnknown **)&pFilter, caGUID.cElems, caGUID.pElems, 0, 0, NULL );
		CoTaskMemFree(caGUID.pElems);
		FilterInfo.pGraph->Release();
	}

	return(S_OK);
}

static void ar2VideoWinDSGetTimeStamp(ARUint32 *t_sec, ARUint32 *t_usec)
{
#ifdef _WIN32
    struct _timeb sys_time;   

    _ftime(&sys_time);   
    *t_sec  = (ARUint32)sys_time.time;
    *t_usec = (ARUint32)sys_time.millitm * 1000;
#else
    struct timeval     time;
    double             tt;
    int                s1, s2;

#if defined(__linux) || defined(__APPLE__)
    gettimeofday( &time, NULL );
#else
    gettimeofday( &time );
#endif
    *t_sec  = time.tv_sec;
    *t_usec = time.tv_usec;
#endif

    return;
}

ARSampleGrabberCB::ARSampleGrabberCB( int flipH, int flipV )
{
	buffer.status = AR2VIDEO_WINDS_STATUS_IDLE;

	if( flipH == 1 ) buffer.flipH = 1;
	else			 buffer.flipH = 0;

	if( flipV == 1 ) buffer.flipV = 1;
	else			 buffer.flipV = 0;
}


STDMETHODIMP ARSampleGrabberCB::QueryInterface(REFIID riid, void **ppvObject)
{
    if (NULL == ppvObject) return E_POINTER;
    if (riid == __uuidof(IUnknown))
    {
        *ppvObject = static_cast<IUnknown*>(this);
            return S_OK;
    }
    if (riid == __uuidof(ISampleGrabberCB))
    {
        *ppvObject = static_cast<ISampleGrabberCB*>(this);
            return S_OK;
    }
    return E_NOTIMPL;
}

STDMETHODIMP ARSampleGrabberCB::SampleCB(double Time, IMediaSample *pSample)
{
    AR2VideoBufferT		tmp;
	HRESULT				hr;

	if( buffer.status == AR2VIDEO_WINDS_STATUS_IDLE ) return S_OK;

	if( buffer.flipH == 0 && buffer.flipV == 0 ) {
		hr = pGrabber->GetCurrentBuffer(&(buffer.bmpBufferSize), (long*)buffer.in.buff);
		if( (FAILED(hr)) ) {
			ARLOGe("Error videoGetImage\n");
		}
	}
	else {
		hr = pGrabber->GetCurrentBuffer(&(buffer.bmpBufferSize), (long*)buffer.pBmpBuffer);
		if( (FAILED(hr)) ) {
			ARLOGe("Error videoGetImage\n");
		}
		ar2VideoWinDSFlipImageRGB24((ARUint8 *)buffer.pBmpBuffer, buffer.in.buff, buffer.width, buffer.height, buffer.flipH, buffer.flipV);
	}
	ar2VideoWinDSGetTimeStamp( &(buffer.in.time_sec), &(buffer.in.time_usec) );
	buffer.in.fillFlag = 1;
	buffer.in.buffLuma = NULL;

	WaitForSingleObject( buffer.buffMutex, INFINITE );
        tmp = buffer.wait;
        buffer.wait = buffer.in;
        buffer.in = tmp;
    ReleaseMutex( buffer.buffMutex );

	return S_OK;
}

STDMETHODIMP ARSampleGrabberCB::BufferCB(double Time, BYTE *pBuffer, long BufferLen)
{
    return S_OK;
}

static void ar2VideoWinDSFlipImageRGB24(ARUint8 *iBuf, ARUint8 *oBuf, int width, int height, int flipH, int flipV)
{
	ARUint8		*pin, *pout;
	int			pixelCount;
	int			i, j;

	pixelCount = width * height;

	if( flipH && flipV ) {
		pin  = iBuf;
		pout = oBuf + (pixelCount-1)*3;

		for( i = 0; i < pixelCount; i++ ) {
			*(pout++) = *(pin++);
			*(pout++) = *(pin++);
			*(pout++) = *(pin++);
			pout -= 6;
        }
	}
	else if( flipH ) {
		pin = iBuf;
		pout = oBuf + (width-1)*3;
		for( j = 0; j < height; j++ ) {
			for( i = 0; i < width; i++ ) {
				*(pout++) = *(pin++);
				*(pout++) = *(pin++);
				*(pout++) = *(pin++);
				pout -= 6;
			}
			pout += width*6;
		}
	}
	else {
		pin  = iBuf;
		for( j = 0; j < height; j++ ) {
			pout = oBuf + ((height-j-1)*width)*3;
			for( i = 0; i < width; i++ ) {
				*(pout++) = *(pin++);
				*(pout++) = *(pin++);
				*(pout++) = *(pin++);
			}
        }
    }

	return;
}

static void ar2VideoWinDSShowProperties2( void *wvid )
{
	AR2VideoParamWinDS2T	*vid = (AR2VideoParamWinDS2T *)wvid;

	vid->showPropertiesFlag = 1;
	ar2VideoWinDSDisplayFilterProperties( vid->pVideoCapFilter );
	vid->showPropertiesFlag = 0;
}

static int ar2VideoWinDSShowProperties( AR2VideoParamWinDST *wvid )
{
	AR2VideoParamWinDS2T	*vid = (AR2VideoParamWinDS2T *)wvid;

	if( vid->showPropertiesFlag == 0 ) {
		_beginthread( ar2VideoWinDSShowProperties2, 0, vid );
	}
	else return -1;

	return 0;
}

#endif // AR_INPUT_WINDOWS_DIRECTSHOW
