/*
 *  videoWinDF.cpp
 *  ARToolKit5
 *
 *  DragonFly/FlyCapture video capture module.
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
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*******************************************************
 *
 * Author: Hirokazu Kato
 *
 *         kato@sys.im.hiroshima-cu.ac.jp
 *
 * Revision: 2.1
 * Date: 2004/01/01
 * Revised 2012-08-05, Philip Lamb.
 *
 *******************************************************/

#include <AR/video.h>

#ifdef AR_INPUT_WINDOWS_DRAGONFLY

#include <AR/ar.h>
#include <assert.h>
#include <windows.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <sys/timeb.h>
#include <PGRFlyCapture.h>

#if defined(_WIN32) && defined(_MSC_VER)
#  if _MSC_VER >= 1600
#    pragma comment(lib,"PGRFlyCapture_v100.lib")
#  elif _MSC_VER == 1500
#    pragma comment(lib,"PGRFlyCapture_v90.lib")
#  else
#    pragma comment(lib,"PGRFlyCapture.lib")
#  endif
#endif

//#define AR_VIDEO_WINDF_FCVIDEOMODE_DEFAULT         FLYCAPTURE_VIDEOMODE_1024x768RGB
//#define AR_VIDEO_WINDF_FCFRAMERATE_DEFAULT         FLYCAPTURE_FRAMERATE_30
#define AR_VIDEO_WINDF_FCVIDEOMODE_DEFAULT         FLYCAPTURE_VIDEOMODE_1600x1200Y8
#define AR_VIDEO_WINDF_FCFRAMERATE_DEFAULT         FLYCAPTURE_FRAMERATE_15

#define AR_VIDEO_WINDF_MAX_CAMS       32

#define _HANDLE_ERROR( error, function ) \
   if( error != FLYCAPTURE_OK ) \
   { \
      ARLOG( "%s: %s\n", function, flycaptureErrorToString( error ) ); \
      return -1; \
   } \

#define _HANDLE_ERROR2( error, function ) \
   if( error != FLYCAPTURE_OK ) \
   { \
      ARLOG( "%s: %s\n", function, flycaptureErrorToString( error ) ); \
      return NULL; \
   } \

#define _HANDLE_ERROR3( error, function ) \
   if( error != FLYCAPTURE_OK ) \
   { \
      ARLOG( "%s: %s\n", function, flycaptureErrorToString( error ) ); \
      return; \
   } \

struct _AR2VideoParamWinDFT {
	int                     status;
	uintptr_t               capture;
    AR2VideoBufferWinDFT    buffer;
	int                     width;
	int                     height;
	AR_PIXEL_FORMAT         pixFormat;
	FlyCaptureVideoMode     fcVideoMode;
	FlyCaptureFrameRate     fcFrameRate;
};

static FlyCaptureError   error;
static FlyCaptureContext context;

static void ar2VideoCaptureWinDF(void *vid);
static void ar2VideoGetTimeStampWinDF(ARUint32 *t_sec, ARUint32 *t_usec);

int ar2VideoDispOptionWinDF( void )
{
    ARLOG(" -device=WinDF\n");
    ARLOG(" -mode=[160x120YUV444|320x240YUV422|640x480YUV411|640x480YUV422|");
    ARLOG("        640x480RGB|640x480Y8|640x480Y16|800x600YUV422|800x600RGB|");
    ARLOG("        800x600Y8|800x600Y16|1024x768YUV422|1024x768RGB|1024x768Y8|");
    ARLOG("        1024x768Y16|1280x960YUV422|1280x960RGB|1280x960Y8|1280x960Y16|");
    ARLOG("        1600x1200YUV422|1600x1200RGB|1600x1200Y8|1600x1200Y16]");
    ARLOG("    specifies input image format.\n");
    ARLOG("    N.B. Not all formats listed may be supported by ARToolKit.\n");
    ARLOG(" -rate=N\n");
    ARLOG("    specifies desired input framerate. \n");
    ARLOG("    (1.875, 3.75, 7.5, 15, 30, 60, 120)\n");
    ARLOG(" -index=N\n");
    ARLOG("    specifies bus index of device to use.\n");
    ARLOG("    (Range is from 0 to number of connected devices minus one.)\n");
    ARLOG("\n");

    return 0;
}

AR2VideoParamWinDFT *ar2VideoOpenWinDF( const char *config )
{
	FlyCaptureInfoEx      arInfo[ AR_VIDEO_WINDF_MAX_CAMS ];
	unsigned int	      uiSize = AR_VIDEO_WINDF_MAX_CAMS;
	unsigned int          uiBusIndex;
    AR2VideoParamWinDFT  *vid;
	FlyCaptureVideoMode   fcVideoMode = AR_VIDEO_WINDF_FCVIDEOMODE_DEFAULT;
	FlyCaptureFrameRate   fcFrameRate = AR_VIDEO_WINDF_FCFRAMERATE_DEFAULT;
	int                   width;
	int                   height;
	AR_PIXEL_FORMAT       pixFormat;
    const char           *a;
    char                  b[256];
    int                   index = 0;

    error = flycaptureBusEnumerateCamerasEx( arInfo, &uiSize );
    _HANDLE_ERROR2( error, "flycaptureBusEnumerateCameras()" );
    
    for( uiBusIndex = 0; uiBusIndex < uiSize; uiBusIndex++ ) {
        ARLOG( "Bus index %u: %s (%u)\n",
              uiBusIndex,
              arInfo[ uiBusIndex ].pszModelName,
              arInfo[ uiBusIndex ].SerialNumber );
    }
    
    a = config;
    if( a != NULL) {
        for(;;) {
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\0' ) break;
            
            if( sscanf(a, "%s", b) == 0 ) break;
            if( strncmp( b, "-mode=", 6 ) == 0 ) {
                if      (strcmp(&b[6], "160x120YUV444") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_160x120YUV444;
                else if (strcmp(&b[6], "320x240YUV422") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_320x240YUV422;
                else if (strcmp(&b[6], "640x480YUV411") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_640x480YUV411;
                else if (strcmp(&b[6], "640x480YUV422") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_640x480YUV422;
                else if (strcmp(&b[6], "640x480RGB") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_640x480RGB;
                else if (strcmp(&b[6], "640x480Y8") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_640x480Y8;
                else if (strcmp(&b[6], "640x480Y16") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_640x480Y16;
                else if (strcmp(&b[6], "800x600YUV422") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_800x600YUV422;
                else if (strcmp(&b[6], "800x600RGB") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_800x600RGB;
                else if (strcmp(&b[6], "800x600Y8") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_800x600Y8;
                else if (strcmp(&b[6], "800x600Y16") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_800x600Y16;
                else if (strcmp(&b[6], "1024x768YUV422") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1024x768YUV422;
                else if (strcmp(&b[6], "1024x768RGB") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1024x768RGB;
                else if (strcmp(&b[6], "1024x768Y8") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1024x768Y8;
                else if (strcmp(&b[6], "1024x768Y16") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1024x768Y16;
                else if (strcmp(&b[6], "1280x960YUV422") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1280x960YUV422;
                else if (strcmp(&b[6], "1280x960RGB") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1280x960RGB;
                else if (strcmp(&b[6], "1280x960Y8") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1280x960Y8;
                else if (strcmp(&b[6], "1280x960Y16") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1280x960Y16;
                else if (strcmp(&b[6], "1600x1200YUV422") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1600x1200YUV422;
                else if (strcmp(&b[6], "1600x1200RGB") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1600x1200RGB;
                else if (strcmp(&b[6], "1600x1200Y8") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1600x1200Y8;
                else if (strcmp(&b[6], "1600x1200Y16") == 0) fcVideoMode = FLYCAPTURE_VIDEOMODE_1600x1200Y16;
                else {
                    ar2VideoDispOptionWinDF();
                    return NULL;
                }
            /*} else if( strncmp( b, "-guid=", 6 ) == 0 ) {
                if( sscanf( &b[6], "%08x%08x", guid[1], guid[0] ) != 2 ) {
                    ar2VideoDispOptionWinDF();
                    return NULL;
                }*/
			} else if( strncmp( b, "-rate=", 6 ) == 0 ) {
                if ( strcmp( &b[6], "1.875" ) == 0 ) fcFrameRate = FLYCAPTURE_FRAMERATE_1_875;
                else if ( strcmp( &b[6], "3.75" ) == 0 ) fcFrameRate = FLYCAPTURE_FRAMERATE_3_75;
                else if ( strcmp( &b[6], "7.5" ) == 0 ) fcFrameRate = FLYCAPTURE_FRAMERATE_7_5;
                else if ( strcmp( &b[6], "15" ) == 0 ) fcFrameRate = FLYCAPTURE_FRAMERATE_15;
                else if ( strcmp( &b[6], "30" ) == 0 ) fcFrameRate = FLYCAPTURE_FRAMERATE_30;
                else if ( strcmp( &b[6], "60" ) == 0 ) fcFrameRate = FLYCAPTURE_FRAMERATE_60;
                else if ( strcmp( &b[6], "120" ) == 0 ) fcFrameRate = FLYCAPTURE_FRAMERATE_120;
                else {
                    ar2VideoDispOptionWinDF();
                    return NULL;
                }
            /*} else if( strcmp( b, "-format7" ) == 0 ) {
                vid->format7 = 1;*/
            } else if (strncmp(b, "-index=", 7) == 0) {
                if (sscanf(&b[7], "%d", &index) != 1) {
                    ar2VideoDispOptionWinDF();
                    return NULL;
                } else if (index < 0 || index >= uiSize) {
                    ARLOGe("Error: device index number %d out of range [0, %d].\n", index, uiSize);
                    return NULL;
                }
            } else if( strcmp( b, "-device=WinDF" ) == 0 ) {
            } else {
                ar2VideoDispOptionWinDF();
                return NULL;
            }
            
            while( *a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

	switch (fcVideoMode) {
        case FLYCAPTURE_VIDEOMODE_160x120YUV444: width=160; height=120; pixFormat=AR_PIXEL_FORMAT_INVALID; break;
        case FLYCAPTURE_VIDEOMODE_320x240YUV422: width=320; height=240; pixFormat=AR_PIXEL_FORMAT_2vuy; break;
        case FLYCAPTURE_VIDEOMODE_640x480YUV411: width=640; height=480; pixFormat=AR_PIXEL_FORMAT_INVALID; break;
        case FLYCAPTURE_VIDEOMODE_640x480YUV422: width=640; height=480; pixFormat=AR_PIXEL_FORMAT_2vuy; break;
        case FLYCAPTURE_VIDEOMODE_640x480RGB: width=640; height=480; pixFormat=AR_PIXEL_FORMAT_RGB; break;
        case FLYCAPTURE_VIDEOMODE_640x480Y8: width=640; height=480; pixFormat=AR_PIXEL_FORMAT_MONO; break;
        case FLYCAPTURE_VIDEOMODE_640x480Y16: width=640; height=480; pixFormat=AR_PIXEL_FORMAT_INVALID; break;
        case FLYCAPTURE_VIDEOMODE_800x600YUV422: width=800; height=600; pixFormat=AR_PIXEL_FORMAT_2vuy; break;
        case FLYCAPTURE_VIDEOMODE_800x600RGB: width=800; height=600; pixFormat=AR_PIXEL_FORMAT_RGB; break;
        case FLYCAPTURE_VIDEOMODE_800x600Y8: width=800; height=600; pixFormat=AR_PIXEL_FORMAT_MONO; break;
        case FLYCAPTURE_VIDEOMODE_800x600Y16: width=800; height=600; pixFormat=AR_PIXEL_FORMAT_INVALID; break;
        case FLYCAPTURE_VIDEOMODE_1024x768YUV422: width=1024; height=768; pixFormat=AR_PIXEL_FORMAT_2vuy; break;
        case FLYCAPTURE_VIDEOMODE_1024x768RGB: width=1024; height=768; pixFormat=AR_PIXEL_FORMAT_RGB; break;
        case FLYCAPTURE_VIDEOMODE_1024x768Y8: width=1024; height=768; pixFormat=AR_PIXEL_FORMAT_MONO; break;
        case FLYCAPTURE_VIDEOMODE_1024x768Y16: width=1024; height=768; pixFormat=AR_PIXEL_FORMAT_INVALID; break;
        case FLYCAPTURE_VIDEOMODE_1280x960YUV422: width=1280; height=960; pixFormat=AR_PIXEL_FORMAT_2vuy; break;
        case FLYCAPTURE_VIDEOMODE_1280x960RGB: width=1280; height=960; pixFormat=AR_PIXEL_FORMAT_RGB; break;
        case FLYCAPTURE_VIDEOMODE_1280x960Y8: width=1280; height=960; pixFormat=AR_PIXEL_FORMAT_MONO; break;
        case FLYCAPTURE_VIDEOMODE_1280x960Y16: width=1280; height=960; pixFormat=AR_PIXEL_FORMAT_INVALID; break;
        case FLYCAPTURE_VIDEOMODE_1600x1200YUV422: width=1600; height=1200; pixFormat=AR_PIXEL_FORMAT_2vuy; break;
        case FLYCAPTURE_VIDEOMODE_1600x1200RGB: width=1600; height=1200; pixFormat=AR_PIXEL_FORMAT_RGB; break;
        case FLYCAPTURE_VIDEOMODE_1600x1200Y8: width=1600; height=1200; pixFormat=AR_PIXEL_FORMAT_MONO; break;
        case FLYCAPTURE_VIDEOMODE_1600x1200Y16: width=1600; height=1200; pixFormat=AR_PIXEL_FORMAT_INVALID; break;
        case FLYCAPTURE_VIDEOMODE_CUSTOM: width=-1; height=-1; pixFormat=AR_PIXEL_FORMAT_INVALID; break;
        case FLYCAPTURE_VIDEOMODE_ANY: width=-1; height=-1; pixFormat=AR_PIXEL_FORMAT_INVALID; break;
        default: width=-1; height=-1; pixFormat=AR_PIXEL_FORMAT_INVALID; break;
	}
	if (width <= 0 || height <= 0 || pixFormat == AR_PIXEL_FORMAT_INVALID) {
		ARLOGe("Error. The requested FlyCaptureVideoMode (%d) is not supported by ARToolKit.\n", fcVideoMode);
		return (NULL);
	}
	arMalloc( vid, AR2VideoParamWinDFT, 1 );
	vid->fcVideoMode = fcVideoMode;
	vid->fcFrameRate = fcFrameRate;
	vid->width = width;
	vid->height = height;
	vid->pixFormat = pixFormat;
	arMalloc( vid->buffer.in.buff,   ARUint8, vid->width*vid->height*arVideoUtilGetPixelSize(vid->pixFormat) );
	arMalloc( vid->buffer.out.buff,  ARUint8, vid->width*vid->height*arVideoUtilGetPixelSize(vid->pixFormat) );
	arMalloc( vid->buffer.wait.buff, ARUint8, vid->width*vid->height*arVideoUtilGetPixelSize(vid->pixFormat) );

    vid->buffer.in.fillFlag   = 0;
    vid->buffer.out.fillFlag  = 0; 
    vid->buffer.wait.fillFlag = 0;
	vid->buffer.buffMutex = CreateMutex( NULL, FALSE, NULL );

    //
    // create the flycapture context.
    //
    error = flycaptureCreateContext( &context );
    _HANDLE_ERROR2( error, "flycaptureCreateContext()" );

    //
    // Initialize the camera.
    //
    ARLOGi( "Initializing camera %u.\n", index );
    error = flycaptureInitialize( context, index );
    _HANDLE_ERROR2( error, "flycaptureInitialize()" );

	bool                pbPresent;
	bool                pbOnePush;
	bool                pbReadOut;
	bool                pbOnOff;
	bool                pbAuto;
	bool				pbManual;
	int                 piMin;
	int                 piMax;
	flycaptureGetCameraPropertyRangeEx(context,FLYCAPTURE_SHUTTER, &pbPresent,&pbOnePush,&pbReadOut,&pbOnOff,&pbAuto,&pbManual,&piMin,&piMax);
	ARLOGi("Shutter: Present=%d OnePush=%d ReadOut=%d OnOff=%d Auto=%d Manual=%d Min=%d Max=%d\n", pbPresent,pbOnePush,pbReadOut,pbOnOff,pbAuto,pbManual,piMin,piMax);
	flycaptureSetCameraProperty(context,FLYCAPTURE_SHUTTER, 200, 0,false);
	flycaptureGetCameraPropertyEx(context,FLYCAPTURE_SHUTTER,&pbOnePush,&pbOnOff,&pbAuto,&piMin,&piMax);
	ARLOGi("Shutter: OnePush=%d OnOff=%d Auto=%d Min=%d Max=%d\n",pbOnePush,pbOnOff,pbAuto,piMin,piMax);






	vid->status = AR2VIDEO_WINDF_STATUS_IDLE;
	vid->capture = _beginthread( ar2VideoCaptureWinDF, 0, vid );
	if( vid->capture == -1 ) {
		error = flycaptureDestroyContext( context );
        free(vid->buffer.in.buff  );
        free(vid->buffer.wait.buff);
        free(vid->buffer.out.buff );
        free( vid );
	    return NULL;
	}

    return vid;
}

int ar2VideoCloseWinDF( AR2VideoParamWinDFT *vid )
{
    vid->status = AR2VIDEO_WINDF_STATUS_STOP;

    while( vid->status != AR2VIDEO_WINDF_STATUS_STOP2 ) Sleep(10);

	CloseHandle( vid->buffer.buffMutex );
    free(vid->buffer.in.buff  );
    free(vid->buffer.wait.buff);
    free(vid->buffer.out.buff );
    free( vid );

    return 0;
} 

int ar2VideoCapStartWinDF( AR2VideoParamWinDFT *vid )
{
    if(vid->status == AR2VIDEO_WINDF_STATUS_RUN){
        ARLOGe("arVideoCapStart has already been called.\n");
        return -1;
    }

    vid->status = AR2VIDEO_WINDF_STATUS_RUN;

    return 0;
}

int ar2VideoCapStopWinDF( AR2VideoParamWinDFT *vid )
{
    if( vid->status != AR2VIDEO_WINDF_STATUS_RUN ) return -1;
    vid->status = AR2VIDEO_WINDF_STATUS_IDLE;

    return 0;
}

AR2VideoBufferT *ar2VideoGetImageWinDF( AR2VideoParamWinDFT *vid )
{
    AR2VideoBufferT   tmp;

	WaitForSingleObject( vid->buffer.buffMutex, INFINITE );
      tmp = vid->buffer.wait;
      vid->buffer.wait = vid->buffer.out;
      vid->buffer.out = tmp;

      vid->buffer.wait.fillFlag = 0;
    ReleaseMutex( vid->buffer.buffMutex );

    return &(vid->buffer.out);
}

static void ar2VideoCaptureWinDF(void *vid2)
{
    FlyCaptureImage			image;
	//FlyCaptureImage			imageOut;
	FlyCaptureColorMethod	colorMethod;
    AR2VideoBufferT			tmp;
	AR2VideoParamWinDFT		*vid;

	vid = (AR2VideoParamWinDFT *)vid2;
    //
    // Start grabbing images in requested mode.
    //
	colorMethod = FLYCAPTURE_NEAREST_NEIGHBOR_FAST;
	//colorMethod = FLYCAPTURE_NEAREST_NEIGHBOR;
	flycaptureGetColorProcessingMethod( context, &colorMethod );
    error = flycaptureStart( 
            context, vid->fcVideoMode, vid->fcFrameRate );
    _HANDLE_ERROR3( error, "flycaptureStart()" );

    while(vid->status != AR2VIDEO_WINDF_STATUS_STOP) {
        if( vid->status == AR2VIDEO_WINDF_STATUS_RUN ) {  

            image.iCols = 0;
            image.iRows = 0;
            error = flycaptureGrabImage2( context, &image );
            _HANDLE_ERROR3( error, "flycaptureGrabImage2()" );

            ar2VideoGetTimeStampWinDF( &(vid->buffer.in.time_sec), &(vid->buffer.in.time_usec) );
			/*
			image.pixelFormat = FLYCAPTURE_RAW8;
			imageOut.pData = vid->buffer.in.buff;
			imageOut.pixelFormat = FLYCAPTURE_RAW8;
            error = flycaptureConvertImage( context, &image, &imageOut );
            _HANDLE_ERROR3( error, "flycaptureConvertImage()" );
			*/
			memcpy(vid->buffer.in.buff, image.pData, vid->width*vid->height*arVideoUtilGetPixelSize(vid->pixFormat));
			vid->buffer.in.fillFlag = 1;

            WaitForSingleObject( vid->buffer.buffMutex, INFINITE );
              tmp = vid->buffer.wait;
              vid->buffer.wait = vid->buffer.in;
              vid->buffer.in = tmp;
            ReleaseMutex( vid->buffer.buffMutex );
        }    
        else if( vid->status == AR2VIDEO_WINDF_STATUS_IDLE ) Sleep(100);
    }

	error = flycaptureDestroyContext( context );
    _HANDLE_ERROR3( error, "flycaptureBusEnumerateCameras()" );

	vid->status = AR2VIDEO_WINDF_STATUS_STOP2;

    return;
}

int ar2VideoGetSizeWinDF(AR2VideoParamWinDFT *vid, int *x,int *y)
{
	if (!vid) return (-1);
	*x = vid->width;
	*y = vid->height;
	return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatWinDF( AR2VideoParamWinDFT *vid )
{
	if (!vid) return (AR_PIXEL_FORMAT_INVALID);
    return (vid->pixFormat);
}

int ar2VideoGetIdWinDF( AR2VideoParamWinDFT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiWinDF( AR2VideoParamWinDFT *vid, int paramName, int *value )
{
    return -1;
}

int ar2VideoSetParamiWinDF( AR2VideoParamWinDFT *vid, int paramName, int  value )
{
    return -1;
}

int ar2VideoGetParamdWinDF( AR2VideoParamWinDFT *vid, int paramName, double *value )
{
    return -1;
}

int ar2VideoSetParamdWinDF( AR2VideoParamWinDFT *vid, int paramName, double  value )
{
    return -1;
}

static void ar2VideoGetTimeStampWinDF(ARUint32 *t_sec, ARUint32 *t_usec)
{
#ifdef _WIN32
    struct _timeb sys_time;   

    _ftime(&sys_time);   
    *t_sec  = sys_time.time;
    *t_usec = sys_time.millitm * 1000;
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

#endif // AR_INPUT_WINDOWS_DRAGONFLY
