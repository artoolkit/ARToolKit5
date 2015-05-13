/*
 *	Video capture subrutine for Linux/libdc1394 devices
 *	author: Kiyoshi Kiyokawa ( kiyo@crl.go.jp )
 *	        Hirokazu Kato ( kato@sys.im.hiroshima-cu.ac.jp )
 *
 *	Revision: 1.0   Date: 2002/01/01
 *
 */
/*
 *	Copyright (c) 2003-2012 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
 *	
 *	Rev		Date		Who		Changes
 *	1.1.0	2003-09-09	PRL		Based on Apple "Son of MungGrab" sample code for QuickTime 6.
 *								Added config option "-fps" to superimpose frame counter on video.
 *								Returns aligned data in ARGB pixel format.
 *  1.2.0   2004-04-28  PRL		Now one thread per video source. Versions of QuickTime
 *								prior to 6.4 are NOT thread safe, and with these earlier
 *								versions, QuickTime toolbox access will be serialised.
 *	1.2.1   2004-06-28  PRL		Support for 2vuy and yuvs pixel formats.
 *  1.3.0   2004-07-13  PRL		Code from Daniel Heckenberg to directly access vDig.
 *  1.3.1   2004-12-07  PRL		Added config option "-pixelformat=" to support pixel format
 *								specification at runtime, with default determined at compile time.
 *	1.4.0	2005-03-08	PRL		Video input settings now saved and restored.
 *  1.4.1   2005-03-15  PRL     QuickTime 6.4 or newer is now required by default. In order
 *								to allow earlier versions, AR_VIDEO_SUPPORT_OLD_QUICKTIME must
 *								be uncommented at compile time.
 *  1.4.2   2007-09-26  GALEOTTI   Made -pixelformat work for format 40 and for Intel Macs.
 *
 */
/*
 *  
 * The functions beginning with names "vdg" adapted with changes from
 * vdigGrab.c, part of the seeSaw project by Daniel Heckenberg.
 *
 * Created by Daniel Heckenberg.
 * Copyright (c) 2004 Daniel Heckenberg. All rights reserved.
 * (danielh.seeSaw<at>cse<dot>unsw<dot>edu<dot>au)
 *  
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the right to use, copy, modify, merge, publish, communicate, sublicence, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * TO THE EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED 
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT 
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NON-INFRINGEMENT.Ê IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// ============================================================================
//	Private includes
// ============================================================================


#include <AR/video.h>

#ifdef AR_INPUT_QUICKTIME

#if defined(_WIN32) && defined(_MSC_VER)
#pragma comment(lib,"QTMLClient.lib")
#pragma comment(lib,"pthreadVC2.lib")
#endif

#include <pthread.h>		// Use pthreads-win32 on Windows.
#include <string.h>			// memcpy()
#ifdef __APPLE__
#  include <Carbon/Carbon.h>
#  include <QuickTime/QuickTime.h>
#  include <CoreServices/CoreServices.h>			// Gestalt()
#  include <unistd.h>		// usleep(), valloc()
#  include <sys/types.h>	// sysctlbyname()
#  include <sys/sysctl.h>	// sysctlbyname()
#  include "videoQuickTimePrivateMacOSX.h"
#  include "QuickDrawCompatibility.h" // As of Mac OS X 10.7 SDK, a bunch of QuickDraw functions are private.
#elif defined(_WIN32)
#  ifndef __MOVIES__
#    include <Movies.h>
#  endif
#  ifndef __QTML__
#    include <QTML.h>
#  endif
#  ifndef __GXMATH__
#    include <GXMath.h>
#  endif
#  ifndef __QUICKTIMECOMPONENTS__
#    include <QuickTimeComponents.h>
#  endif
#  ifndef __MEDIAHANDLERS__
#    include <MediaHandlers.h>
#  endif
#else
#  error
#endif


// ============================================================================
//	Private definitions
// ============================================================================
#ifdef _WIN32
#  define valloc malloc
#  define usleep(t) Sleep((DWORD)(t/1000u));
#  define AR_VIDEO_SUPPORT_OLD_QUICKTIME		// QuickTime for Windows is not-thread safe.
#else
//#  define AR_VIDEO_SUPPORT_OLD_QUICKTIME		// Uncomment to allow use of non-thread safe QuickTime (pre-6.4).
#endif



#define AR_VIDEO_QUICKTIME_IDLE_INTERVAL_MILLISECONDS_MIN		5L
#define AR_VIDEO_QUICKTIME_IDLE_INTERVAL_MILLISECONDS_MAX		20L

#define AR_VIDEO_QUICKTIME_STATUS_BIT_READY   0x01			// Clear when no new frame is ready, set when a new frame is ready.
#define AR_VIDEO_QUICKTIME_STATUS_BIT_BUFFER  0x02			// Clear when buffer 1 is valid for writes, set when buffer 2 is valid for writes. 

// Early Mac OS X implementations of pthreads failed to define PTHREAD_CANCELED.
#ifdef PTHREAD_CANCELED
#  define AR_PTHREAD_CANCELLED PTHREAD_CANCELED
#else
#  define AR_PTHREAD_CANCELLED ((void *) 1);
#endif

// pthreads-win32 cleanup functions must be declared using the cdecl calling convention.
#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(__MINGW32__)
#  ifndef ARVIDEO_APIENTRY
#    define ARVIDEO_APIENTRY __cdecl
#  endif
#elif defined(__CYGWIN__) || defined(__MINGW32__)
#  ifndef ARVIDEO_APIENTRY
#    define ARVIDEO_APIENTRY __attribute__ ((__cdecl__))
#  endif
#else // non-Win32 case.
#  ifndef ARVIDEO_APIENTRY
#    define ARVIDEO_APIENTRY
#  endif
#endif

// ============================================================================
//	Private types
// ============================================================================

struct _VdigGrab
{
	// State
	int					isPreflighted;
	int					isGrabbing;
	int					isRecording;
	
	// QT Components
	SeqGrabComponent	seqGrab; 
	SGChannel			sgchanVideo;
	ComponentInstance   vdCompInst;
	
	// Device settings
	ImageDescriptionHandle	vdImageDesc;
	Rect					vdDigitizerRect;		
	
	// Destination Settings
	CGrafPtr				dstPort;
	ImageSequence			dstImageSeq;
	
	// Compression settings
	short				cpDepth;
	CompressorComponent cpCompressor;
	CodecQ				cpSpatialQuality;
	CodecQ				cpTemporalQuality;
	long				cpKeyFrameRate;
	Fixed				cpFrameRate;
};
typedef struct _VdigGrab VdigGrab;
typedef struct _VdigGrab *VdigGrabRef;

struct _AR2VideoParamQuickTimeT {
	int						itsAMovie;
	AR2VideoBufferT			arVideoBuffer;
	// Parameters for input based on sequence grabber.
    int						width;
    int						height;
    Rect					theRect;
    GWorldPtr				pGWorld;
    int						status;
	int						showFPS;
	TimeValue				lastTime;
	long					frameCount;
	TimeScale				timeScale;
	pthread_t				thread;			// PRL.
	pthread_mutex_t			bufMutex;		// PRL.
	pthread_cond_t			condition;		// PRL.
	int						threadRunning;  // PRL.
	OSType					pixFormat;		// PRL.
	long					rowBytes;		// PRL.
	long					bufSize;		// PRL.
	ARUint8*				bufPixels;		// PRL.
	int						bufCopyFlag;	// PRL
	ARUint8*				bufPixelsCopy1; // PRL.
	ARUint8*				bufPixelsCopy2; // PRL.
	int						grabber;		// PRL.
	MatrixRecordPtr			scaleMatrixPtr; // PRL.
	VdigGrabRef				pVdg;			// DH (seeSaw).
	long					milliSecPerTimer; // DH (seeSaw).
	long					milliSecPerFrame; // DH (seeSaw).
	Fixed					frameRate;		// DH (seeSaw).
	long					bytesPerSecond; // DH (seeSaw).
	ImageDescriptionHandle  vdImageDesc;	// DH (seeSaw).
	// Parameters for input based on movies.
	AR_VIDEO_QUICKTIME_MOVIE_t movie;
};
typedef struct _AR2VideoParamQuickTimeT *AR2VideoParamQuickTimeTRef;

// ============================================================================
//	Private global variables
// ============================================================================

static unsigned int		gGrabberActiveCount = 0;
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
static pthread_mutex_t  gGrabberQuickTimeMutex;
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
static unsigned int		gMoviesActiveCount = 0;

#pragma mark -

// ============================================================================
//	Private functions
// ============================================================================

// --------------------
// MakeSequenceGrabber  (adapted from Apple mung sample)
//
static SeqGrabComponent MakeSequenceGrabber(WindowRef pWindow, const int grabber)
{
	SeqGrabComponent	seqGrab = NULL;
	ComponentResult		err = noErr;
	ComponentDescription cDesc;
	long				cCount;
	Component			c;
	int					i;
	
	// Open the sequence grabber.
	cDesc.componentType = SeqGrabComponentType;
	cDesc.componentSubType = 0L; // Could use subtype vdSubtypeIIDC for IIDC-only cameras (i.e. exclude DV and other cameras.)
	cDesc.componentManufacturer = cDesc.componentFlags = cDesc.componentFlagsMask = 0L;
	cCount = CountComponents(&cDesc);
	ARLOGi("Opening sequence grabber %d of %ld.\n", grabber, cCount);
	c = 0;
	for (i = 1; (c = FindNextComponent(c, &cDesc)) != 0; i++) {
		// Could call GetComponentInfo() here to get more info on this SeqGrabComponentType component.
		// Is this the grabber requested?
		if (i == grabber) {
			seqGrab = OpenComponent(c);
		}
	}
    if (!seqGrab) {
		ARLOGe("MakeSequenceGrabber(): Failed to open a sequence grabber component.\n");
		goto endFunc;
	}
	
   	// initialize the default sequence grabber component
   	if ((err = SGInitialize(seqGrab))) {
		ARLOGe("MakeSequenceGrabber(): SGInitialize err=%ld\n", err);
		goto endFunc;
	}
	
	// This should be defaulted to the current port according to QT doco
	if ((err = SGSetGWorld(seqGrab, GetWindowPort(pWindow), NULL))) {
		ARLOGe("MakeSequenceGrabber(): SGSetGWorld err=%ld\n", err);
		goto endFunc;
	}
	
	// specify the destination data reference for a record operation
	// tell it we're not making a movie
	// if the flag seqGrabDontMakeMovie is used, the sequence grabber still calls
	// your data function, but does not write any data to the movie file
	// writeType will always be set to seqGrabWriteAppend
  	if ((err = SGSetDataRef(seqGrab, 0, 0, seqGrabDontMakeMovie))) {
		ARLOGe("MakeSequenceGrabber(): SGSetDataRef err=%ld\n", err);
		goto endFunc;
	}
	
endFunc:	
	if (err && (seqGrab != NULL)) { // clean up on failure
		CloseComponent(seqGrab);
		seqGrab = NULL;
	}
    
	return (seqGrab);
}


// --------------------
// MakeSequenceGrabChannel (adapted from Apple mung sample)
//
static ComponentResult MakeSequenceGrabChannel(SeqGrabComponent seqGrab, SGChannel* psgchanVideo)
{
    long  flags = 0;
    ComponentResult err = noErr;
    
    if ((err = SGNewChannel(seqGrab, VideoMediaType, psgchanVideo))) {
		if (err == couldntGetRequiredComponent) {
			ARLOGe("ERROR: No camera connected. Please connect a camera and re-try.\n");
		} else {
			ARLOGe("MakeSequenceGrabChannel(): SGNewChannel err=%ld\n", err);
		}
		goto endFunc;
	}
	
	//err = SGSetChannelBounds(*sgchanVideo, rect);
   	// set usage for new video channel to avoid playthrough
	// note we don't set seqGrabPlayDuringRecord
	if ((err = SGSetChannelUsage(*psgchanVideo, flags | seqGrabRecord))) {
		ARLOGe("MakeSequenceGrabChannel(): SGSetChannelUsage err=%ld\n", err);
		goto endFunc;
	}
	
endFunc:
	if ((err != noErr) && psgchanVideo) {
		// clean up on failure
		SGDisposeChannel(seqGrab, *psgchanVideo);
		*psgchanVideo = NULL;
	}
	
	return err;
}

static ComponentResult vdgGetSettings(VdigGrab* pVdg)
{	
	ComponentResult err;
	
	// Extract information from the SG
    if ((err = SGGetVideoCompressor (pVdg->sgchanVideo, 
									&pVdg->cpDepth,
									&pVdg->cpCompressor,
									&pVdg->cpSpatialQuality, 
									&pVdg->cpTemporalQuality, 
									&pVdg->cpKeyFrameRate))) {
		ARLOGe("SGGetVideoCompressor err=%ld\n", err);
		goto endFunc;
	}
    
	if ((err = SGGetFrameRate(pVdg->sgchanVideo, &pVdg->cpFrameRate))) {
		ARLOGe("SGGetFrameRate err=%ld\n", err);
		goto endFunc;
	}
	
	// Get the selected vdig from the SG
    if (!(pVdg->vdCompInst = SGGetVideoDigitizerComponent(pVdg->sgchanVideo))) {
		ARLOGe("SGGetVideoDigitizerComponent error\n");
		goto endFunc;
	}
	
endFunc:
	return (err);
}

#pragma mark -

VdigGrabRef vdgAllocAndInit(const int grabber)
{
	VdigGrabRef pVdg = NULL;
	ComponentResult err;
	
	// Allocate the grabber structure
	arMallocClear(pVdg, VdigGrab, 1);
	
	if (!(pVdg->seqGrab = MakeSequenceGrabber(NULL, grabber))) {
		ARLOGe("MakeSequenceGrabber error.\n"); 
		free(pVdg);
		return (NULL);
	}
	
	if ((err = MakeSequenceGrabChannel(pVdg->seqGrab, &pVdg->sgchanVideo))) {
		if (err != couldntGetRequiredComponent) ARLOGe("MakeSequenceGrabChannel err=%ld.\n", err); 
		free(pVdg);
		return (NULL);
	}
	
	return (pVdg);
}

static ComponentResult vdgRequestSettings(VdigGrab* pVdg, const int showDialog, const int standardDialog, const int inputIndex)
{
	ComponentResult err;
	
	// Use the SG Dialog to allow the user to select device and compression settings
#ifdef __APPLE__
	if ((err = RequestSGSettings(inputIndex, pVdg->seqGrab, pVdg->sgchanVideo, showDialog, standardDialog) != noErr)) {
		ARLOGe("RequestSGSettings err=%ld\n", err); 
		goto endFunc;
	}
#else
	if (showDialog) {
		if ((err = SGSettingsDialog(pVdg->seqGrab, pVdg->sgchanVideo, 0, 0, seqGrabSettingsPreviewOnly, NULL, 0L)) != noErr) {
			ARLOGe("SGSettingsDialog err=%ld\n", err); 
			goto endFunc;
		}
	} 
#endif
	
	if ((err = vdgGetSettings(pVdg))) {
		ARLOGe("vdgGetSettings err=%ld\n", err); 
		goto endFunc;
	}
	
endFunc:
		return err;
}

/*static VideoDigitizerError vdgGetDeviceNameAndFlags(VdigGrab* pVdg, char* szName, long* pBuffSize, UInt32* pVdFlags)
{
	VideoDigitizerError err;
	Str255	vdName; // Pascal string (first byte is string length).
    UInt32	vdFlags;
	
	if (!pBuffSize) {
		ARLOGe("vdgGetDeviceName: NULL pointer error\n");
		err = (VideoDigitizerError)qtParamErr; 
		goto endFunc;
	}
	
	if ((err = VDGetDeviceNameAndFlags(  pVdg->vdCompInst,
										vdName,
										&vdFlags))) {
		ARLOGe("VDGetDeviceNameAndFlags err=%ld\n", err);
		*pBuffSize = 0; 
		goto endFunc;
	}
	
	if (szName) {
		int copyLen = (*pBuffSize-1 < vdName[0] ? *pBuffSize-1 : vdName[0]);
		
		strncpy(szName, (char *)vdName+1, copyLen);
		szName[copyLen] = '\0';
		
		*pBuffSize = copyLen + 1;
	} else {
		*pBuffSize = vdName[0] + 1;
	} 
	
	if (pVdFlags)
		*pVdFlags = vdFlags;
	
endFunc:
		return err;
}
*/

static OSErr vdgSetDestination(  VdigGrab* pVdg,
					CGrafPtr  dstPort )
{
	pVdg->dstPort = dstPort;
	return noErr;
}

static VideoDigitizerError vdgPreflightGrabbing(VdigGrab* pVdg)
{
	/* from Steve Sisak (on quicktime-api list):
	A much more optimal case, if you're doing it yourself is:
	
	VDGetDigitizerInfo() // make sure this is a compressed source only
	VDGetCompressTypes() // tells you the supported types
	VDGetMaxSourceRect() // returns full-size rectangle (sensor size)
	VDSetDigitizerRect() // determines cropping
	
	VDSetCompressionOnOff(true)
	
    VDSetFrameRate()         // set to 0 for default
    VDSetCompression()       // compresstype=0 means default
    VDGetImageDescription()  // find out image format
    VDGetDigitizerRect()     // find out if vdig is cropping for you
    VDResetCompressSequence()
	
    (grab frames here)
	
	VDSetCompressionOnOff(false)
	*/
	VideoDigitizerError err;
    Rect maxRect;
	
	DigitizerInfo info;
	
	// make sure this is a compressed source only
	if ((err = VDGetDigitizerInfo(pVdg->vdCompInst, &info))) {
		ARLOGe("vdgPreflightGrabbing(): VDGetDigitizerInfo err=%ld\n", err);
		goto endFunc;
	} else {
		if (!(info.outputCapabilityFlags & digiOutDoesCompress)) {
			ARLOGe("vdgPreflightGrabbing(): VDGetDigitizerInfo reports device is not a compressed source.\n");
			err = digiUnimpErr; // We don't support non-compressed sources.
			goto endFunc;
		}
	}
	
	//  VDGetCompressTypes() // tells you the supported types
	
	// Tell the vDig we're starting to change several settings.
	// Apple's SoftVDig doesn't seem to like these calls.
	if ((err = VDCaptureStateChanging(pVdg->vdCompInst, vdFlagCaptureSetSettingsBegin))) {
		if (err != digiUnimpErr) ARLOGe("vdgPreflightGrabbing(): VDCaptureStateChanging err=%ld (Ignored.)\n", err);
		//goto endFunc;
	}

	if ((err = VDGetMaxSrcRect(pVdg->vdCompInst, currentIn, &maxRect))) {
		ARLOGe("vdgPreflightGrabbing(): VDGetMaxSrcRect err=%ld (Ignored.)\n", err);
		//goto endFunc;
	}
	
	// Try to set maximum capture size ... is this necessary as we're setting the 
	// rectangle in the VDSetCompression call later?  I suppose that it is, as
	// we're setting digitization size rather than compression size here...
	// Apple vdigs don't like this call	
	if ((err = VDSetDigitizerRect( pVdg->vdCompInst, &maxRect))) {
		if (err != digiUnimpErr) ARLOGe("vdgPreflightGrabbing(): VDSetDigitizerRect err=%ld (Ignored.)\n", err);
		//goto endFunc;		
	}
	
	if ((err = VDSetCompressionOnOff( pVdg->vdCompInst, 1))) {
		if (err != digiUnimpErr) ARLOGe("vdgPreflightGrabbing(): VDSetCompressionOnOff err=%ld (Ignored.)\n", err);
		//goto endFunc;		
	}
	
	// We could try to force the frame rate here... necessary for ASC softvdig
	if ((err = VDSetFrameRate(pVdg->vdCompInst, 0))) {
		if (err != digiUnimpErr) ARLOGe("vdgPreflightGrabbing(): VDSetFrameRate err=%ld (Ignored.)\n", err);
		//goto endFunc;		
	}
	
	// try to set a format that matches our target
	// necessary for ASC softvdig (even if it doesn't support
	// the requested codec)
	// note that for the Apple IIDC vdig in 10.3 if we request yuv2 explicitly
	// we'll get 320x240 frames returned but if we leave codecType as 0
	// we'll get 640x480 frames returned instead (which use 4:1:1 encoding on
	// the wire rather than 4:2:2)
    if ((err = VDSetCompression(pVdg->vdCompInst,
							   0, //'yuv2'
							   0,	
							   &maxRect, 
							   0, //codecNormalQuality,
							   0, //codecNormalQuality,
							   0))) {
		if (err != digiUnimpErr) ARLOGe("vdgPreflightGrabbing(): VDSetCompression err=%ld (Ignored.)\n", err);
		//goto endFunc;			
	}
	
	if ((err = VDCaptureStateChanging(pVdg->vdCompInst, vdFlagCaptureLowLatency))) {
		if (err != digiUnimpErr) ARLOGe("vdgPreflightGrabbing(): VDCaptureStateChanging err=%ld (Ignored.)\n", err);
		//goto endFunc;	   
	}

	// Tell the vDig we've finished changing settings.
	if ((err = VDCaptureStateChanging(pVdg->vdCompInst, vdFlagCaptureSetSettingsEnd))) {
		if (err != digiUnimpErr)  ARLOGe("vdgPreflightGrabbing(): VDCaptureStateChanging err=%ld (Ignored.)\n", err);
		//goto endFunc;	   
	}

	if ((err = VDResetCompressSequence( pVdg->vdCompInst))) {
		if (err != digiUnimpErr) ARLOGe("vdgPreflightGrabbing(): VDResetCompressSequence err=%ld (Ignored.)\n", err);
		//goto endFunc;	   
	}
	
	pVdg->vdImageDesc = (ImageDescriptionHandle)NewHandle(0);
	if ((err = VDGetImageDescription(pVdg->vdCompInst, pVdg->vdImageDesc))) {
		ARLOGe("vdgPreflightGrabbing(): VDGetImageDescription err=%ld (Ignored.)\n", err);
		//goto endFunc;	   
	}
	
	// From Steve Sisak: find out if Digitizer is cropping for you.
	if ((err = VDGetDigitizerRect(pVdg->vdCompInst, &pVdg->vdDigitizerRect))) {
		ARLOGe("vdgPreflightGrabbing(): VDGetDigitizerRect err=%ld (Ignored.)\n", err);
		//goto endFunc;
	}
	
	pVdg->isPreflighted = 1;
	
endFunc:
	return (err);
}

static VideoDigitizerError vdgGetDataRate( VdigGrab*   pVdg, 
				long*		pMilliSecPerFrame,
				Fixed*      pFramesPerSecond,
				long*       pBytesPerSecond)
{
	VideoDigitizerError err;
	
	if ((err = VDGetDataRate( pVdg->vdCompInst, 
							 pMilliSecPerFrame,
							 pFramesPerSecond,
							 pBytesPerSecond))) {
		ARLOGe("vdgGetDataRate(): VDGetDataRate err=%ld\n", err);
		goto endFunc;		
	}
	
endFunc:	
	return (err);
}

static VideoDigitizerError vdgGetImageDescription( VdigGrab* pVdg,
						ImageDescriptionHandle vdImageDesc )
{
	VideoDigitizerError err;
	
	if ((err = VDGetImageDescription( pVdg->vdCompInst, vdImageDesc)))
	{
		ARLOGe("VDGetImageDescription err=%ld\n", err);
		goto endFunc;		
	}
	
endFunc:	
		return err;
}

static OSErr vdgDecompressionSequenceBegin(  VdigGrab* pVdg,
								CGrafPtr dstPort, 
								Rect* pDstRect,
								MatrixRecord* pDstScaleMatrix )
{
	OSErr err;
	
	// 	Rect				   sourceRect = pMungData->bounds;
	//	MatrixRecord		   scaleMatrix;	
	
  	// !HACK! Different conversions are used for these two equivalent types
	// so we force the cType so that the more efficient path is used
	if ((*pVdg->vdImageDesc)->cType == FOUR_CHAR_CODE('yuv2'))
		(*pVdg->vdImageDesc)->cType = FOUR_CHAR_CODE('yuvu'); // kYUVUPixelFormat
	
	// make a scaling matrix for the sequence
	//	sourceRect.right = (*pVdg->vdImageDesc)->width;
	//	sourceRect.bottom = (*pVdg->vdImageDesc)->height;
	//	RectMatrix(&scaleMatrix, &sourceRect, &pMungData->bounds);
	
    // begin the process of decompressing a sequence of frames
    // this is a set-up call and is only called once for the sequence - the ICM will interrogate different codecs
    // and construct a suitable decompression chain, as this is a time consuming process we don't want to do this
    // once per frame (eg. by using DecompressImage)
    // for more information see Ice Floe #8 http://developer.apple.com/quicktime/icefloe/dispatch008.html
    // the destination is specified as the GWorld
	if ((err = DecompressSequenceBeginS(    &pVdg->dstImageSeq,	// pointer to field to receive unique ID for sequence.
										   pVdg->vdImageDesc,	// handle to image description structure.
										   0,					// pointer to compressed image data.
										   0,					// size of the buffer.
										   dstPort,				// port for the DESTINATION image
										   NULL,				// graphics device handle, if port is set, set to NULL
										   NULL, //&sourceRect  // source rectangle defining the portion of the image to decompress 
										   pDstScaleMatrix,		// transformation matrix
										   srcCopy,				// transfer mode specifier
										   (RgnHandle)NULL,		// clipping region in dest. coordinate system to use as a mask
										   0L,					// flags
										   codecHighQuality, //codecNormalQuality   // accuracy in decompression
										   bestSpeedCodec))) //anyCodec  bestSpeedCodec  // compressor identifier or special identifiers ie. bestSpeedCodec
	{
		ARLOGe("DecompressSequenceBeginS err=%d\n", err);
		goto endFunc;
	}
		  
endFunc:	
		return err;
}

static OSErr vdgDecompressionSequenceWhen(   VdigGrab* pVdg,
								Ptr theData,
								long dataSize)
{
	OSErr err;
	CodecFlags	ignore = 0;
	
	if ((err = DecompressSequenceFrameWhen(   pVdg->dstImageSeq,	// sequence ID returned by DecompressSequenceBegin.
											theData,			// pointer to compressed image data.
											dataSize,			// size of the buffer.
											0,					// in flags.
											&ignore,			// out flags.
											NULL,				// async completion proc.
											NULL )))				// frame timing information.
	{
		ARLOGe("DecompressSequenceFrameWhen err=%d\n", err);
		goto endFunc;
	}
	
endFunc:
		return err;
}

static OSErr vdgDecompressionSequenceEnd( VdigGrab* pVdg )
{
	OSErr err;
	
	if (!pVdg->dstImageSeq)
	{
		ARLOGe("vdgDestroyDecompressionSequence NULL sequence\n");
		err = qtParamErr; 
		goto endFunc;
	}
	
	if ((err = CDSequenceEnd(pVdg->dstImageSeq)))
	{
		ARLOGe("CDSequenceEnd err=%d\n", err);
		goto endFunc;
	}
	
	pVdg->dstImageSeq = 0;
	
endFunc:
		return err;
}

static VideoDigitizerError vdgStartGrabbing(VdigGrab*   pVdg, MatrixRecord* pDstScaleMatrix)
{
	VideoDigitizerError err;
	
	if (!pVdg->isPreflighted)
	{
		ARLOGe("vdgStartGrabbing called without previous successful vdgPreflightGrabbing()\n");
		err = (VideoDigitizerError)badCallOrderErr; 
		goto endFunc;	
	}
	
    if ((err = VDCompressOneFrameAsync( pVdg->vdCompInst )))
	{
		ARLOGe("VDCompressOneFrameAsync err=%ld\n", err);
		goto endFunc;	
	}
	
	if ((err = vdgDecompressionSequenceBegin( pVdg, pVdg->dstPort, NULL, pDstScaleMatrix )))
	{
		ARLOGe("vdgDecompressionSequenceBegin err=%ld\n", err);
		goto endFunc;	
	}
	
	pVdg->isGrabbing = 1;
	
endFunc:
		return err;
}

static VideoDigitizerError vdgStopGrabbing(VdigGrab* pVdg)
{
	VideoDigitizerError err;
	
	if ((err = VDSetCompressionOnOff( pVdg->vdCompInst, 0)))
	{
		ARLOGe("VDSetCompressionOnOff err=%ld\n", err);
		//		goto endFunc;		
	}
	
	if ((err = (VideoDigitizerError)vdgDecompressionSequenceEnd(pVdg)))
	{
		ARLOGe("vdgDecompressionSequenceEnd err=%ld\n", err);
		//		goto endFunc;
	}
	
	pVdg->isGrabbing = 0;
	
	//endFunc:
	return err;
}

static bool vdgIsGrabbing(VdigGrab* pVdg)
{
	return (pVdg->isGrabbing != 0);
}

static VideoDigitizerError vdgPoll(	VdigGrab*   pVdg,
									UInt8*		pQueuedFrameCount,
									Ptr*		pTheData,
									long*		pDataSize,
									UInt8*		pSimilarity,
									TimeRecord*	pTime )
{
	VideoDigitizerError err;
	
	if (!pVdg->isGrabbing)
	{ 
		ARLOGe("vdgGetFrame error: not grabbing\n");
		err = (VideoDigitizerError)qtParamErr; 
		goto endFunc;
	}
	
    if ((err = VDCompressDone(	pVdg->vdCompInst,
								pQueuedFrameCount,
								pTheData,
								pDataSize,
								pSimilarity,
								pTime )))
	{
		ARLOGe("VDCompressDone err=%ld\n", err);
		goto endFunc;
	}
	
	// Overlapped grabbing
    if (*pQueuedFrameCount)
    {
		if ((err = VDCompressOneFrameAsync(pVdg->vdCompInst)))
		{
			ARLOGe("VDCompressOneFrameAsync err=%ld\n", err);
			goto endFunc;		
		}
	}
	
endFunc:
		return err;
}

static VideoDigitizerError vdgReleaseBuffer(VdigGrab*   pVdg, Ptr theData)
{
	VideoDigitizerError err;
	
	if ((err = VDReleaseCompressBuffer(pVdg->vdCompInst, theData)))
	{
		ARLOGe("VDReleaseCompressBuffer err=%ld\n", err);
		goto endFunc;		
	}
	
endFunc:
		return err;
}

static VideoDigitizerError vdgIdle(VdigGrab* pVdg, int*  pIsUpdated)
{
	VideoDigitizerError err;
	
    UInt8 		queuedFrameCount;
    Ptr			theData;
    long		dataSize;
    UInt8		similarity;
    TimeRecord	time;
	
	*pIsUpdated = 0;
	
	// should be while?
	if ( !(err = vdgPoll( pVdg,
						  &queuedFrameCount,
						  &theData,
						  &dataSize,
						  &similarity,
						  &time))
		 && queuedFrameCount)
	{
		*pIsUpdated = 1;
		
		// Decompress the sequence
		if ((err = (VideoDigitizerError)vdgDecompressionSequenceWhen( pVdg,
												theData,
												dataSize)))
		{
			ARLOGe("vdgDecompressionSequenceWhen err=%ld\n", err);
			//			goto endFunc;	
		}
		
		// return the buffer
		if ((err = vdgReleaseBuffer(pVdg, theData)))
		{
			ARLOGe("vdgReleaseBuffer err=%ld\n", err);
			//			goto endFunc;
		}
	}
	
	if (err)
	{
		ARLOGe("vdgPoll err=%ld\n", err);
		goto endFunc;
	}
	
endFunc:
		return err;
}

static ComponentResult vdgReleaseAndDealloc(VdigGrab* pVdg)
{
	ComponentResult err = noErr;		
	
	if (pVdg->vdImageDesc) {
		DisposeHandle((Handle)pVdg->vdImageDesc);
		pVdg->vdImageDesc = NULL;
	}
	
	if (pVdg->vdCompInst) {
		if ((err = CloseComponent(pVdg->vdCompInst)))
			ARLOGe("CloseComponent err=%ld\n", err);		
		pVdg->vdCompInst = NULL;
	}
	
	if (pVdg->sgchanVideo) {
		if ((err = SGDisposeChannel(pVdg->seqGrab, pVdg->sgchanVideo)))
			ARLOGe("SGDisposeChannel err=%ld\n", err);	
		pVdg->sgchanVideo = NULL;
	}
	
	if (pVdg->seqGrab) {
		if ((err = CloseComponent(pVdg->seqGrab)))
			ARLOGe("CloseComponent err=%ld\n", err);		
		pVdg->seqGrab = NULL;
	}

	if (pVdg) {
		free(pVdg);
		pVdg = NULL;
	}

	return err;
}

#pragma mark -
static int ar2VideoInternalLock(pthread_mutex_t *mutex)
{
	int err;
	
	// Ready to access data, so lock access to the data.
	if ((err = pthread_mutex_lock(mutex)) != 0) {
		perror("ar2VideoInternalLock(): Error locking mutex");
		return (0);
	}
	return (1);
}

#if 0
static int ar2VideoInternalTryLock(pthread_mutex_t *mutex)
{
	int err;
	
	// Ready to access data, so lock access to the data.
	if ((err = pthread_mutex_trylock(mutex)) != 0) {
		if (err == EBUSY) return (-1);
		perror("ar2VideoInternalTryLock(): Error locking mutex");
		return (0);
	}
	return (1);
}
#endif

static int ar2VideoInternalUnlock(pthread_mutex_t *mutex)
{
	int err;
	
	// Our access is done, so unlock access to the data.
	if ((err = pthread_mutex_unlock(mutex)) != 0) {
		perror("ar2VideoInternalUnlock(): Error unlocking mutex");
		return (0);
	}
	return (1);
}

static void ARVIDEO_APIENTRY ar2VideoInternalThreadCleanup(void *arg)
{
	AR2VideoParamQuickTimeT *vid;
	
	vid = (AR2VideoParamQuickTimeT *)arg;
	ar2VideoInternalUnlock(&(vid->bufMutex)); // A cancelled thread shouldn't leave mutexes locked.
#ifndef AR_VIDEO_SUPPORT_OLD_QUICKTIME
	ExitMoviesOnThread();
#else
	ar2VideoInternalUnlock(&gGrabberQuickTimeMutex);
#endif // !AR_VIDEO_SUPPORT_OLD_QUICKTIME
}

//
// This function will run in a separate pthread.
// Its sole function is to call vdgIdle() on a regular basis during a capture operation.
// It should be terminated by a call pthread_cancel() from the instantiating thread.
//
static void *ar2VideoInternalThread(void *arg)
{
#ifndef AR_VIDEO_SUPPORT_OLD_QUICKTIME
	OSErr				err_o;
#else
	int					weLocked = 0;
#endif // !AR_VIDEO_SUPPORT_OLD_QUICKTIME
	AR2VideoParamQuickTimeT		*vid;

	ComponentResult		err;
	int					isUpdated = 0;

	
#ifndef AR_VIDEO_SUPPORT_OLD_QUICKTIME
	// Signal to QuickTime that this is a separate thread.
	if ((err_o = EnterMoviesOnThread(0)) != noErr) {
		ARLOGe("ar2VideoInternalThread(): Error %d initing QuickTime for this thread.\n", err_o);
		return (NULL);
	}
#endif // !AR_VIDEO_SUPPORT_OLD_QUICKTIME

	// Register our cleanup function, with arg as arg.
	pthread_cleanup_push(ar2VideoInternalThreadCleanup, arg);
	
	vid = (AR2VideoParamQuickTimeT *)arg;		// Cast the thread start arg to the correct type.
	
	// Have to get the lock now, to guarantee vdgIdle() exclusive access to *vid.
	// The lock is released while we are blocked inside pthread_cond_timedwait(),
	// and during that time ar2VideoGetImage() (and therefore OpenGL) can access
	// *vid exclusively.
    if (!vid) {
		ARLOGe("ar2VideoInternalThread(): vid is NULL, exiting.\n");
    } else if (!ar2VideoInternalLock(&(vid->bufMutex))) {
		ARLOGe("ar2VideoInternalThread(): Unable to lock mutex, exiting.\n");
	} else {
        
        while (vdgIsGrabbing(vid->pVdg)) {
        
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
            // Get a lock to access QuickTime (for SGIdle()), but only if more than one thread is running.
            if (gGrabberActiveCount > 1) {
                if (!ar2VideoInternalLock(&gGrabberQuickTimeMutex)) {
                    ARLOGe("ar2VideoInternalThread(): Unable to lock mutex (for QuickTime), exiting.\n");
                    break;
                }
                weLocked = 1;
            }
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
            
            if ((err = vdgIdle(vid->pVdg, &isUpdated)) != noErr) {
                // In QT 4 you would always encounter a cDepthErr error after a user drags
                // the window, this failure condition has been greatly relaxed in QT 5
                // it may still occur but should only apply to vDigs that really control
                // the screen.
                // You don't always know where these errors originate from, some may come
                // from the VDig.
                ARLOGe("vdgIdle err=%ld.\n", err);
                break;
            }
            
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
            // vdgIdle() is done, unlock our hold on QuickTime if we locked it.
            if (weLocked) {
                if (!ar2VideoInternalUnlock(&gGrabberQuickTimeMutex)) {
                    ARLOGe("ar2VideoInternalThread(): Unable to unlock mutex (for QuickTime), exiting.\n");
                    break;
                }
                weLocked = 0;
            }
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
            
            if (isUpdated) {
                // Write status information onto the frame if so desired.
                if (vid->showFPS) {
                    
                    // Variables for fps counter.
                    //float				fps = 0;
                    //float				averagefps = 0;
                    char				status[64];
                    
                    // Reset frame and time counters after a stop/start.
                    /*
                     if (vid->lastTime > time) {
                        vid->lastTime = 0;
                        vid->frameCount = 0;
                    }
                    */
                    vid->frameCount++;
                    // If first time here, get the time scale.
                    /*
                    if (vid->timeScale == 0) {
                        if ((err = SGGetChannelTimeScale(c, &vid->timeScale)) != noErr) {
                            ARLOGe("SGGetChannelTimeScale err=%ld.\n", err);
                        }
                    }
                    */
                    //fps = (float)vid->timeScale / (float)(time - vid->lastTime);
                    //averagefps = (float)vid->frameCount * (float)vid->timeScale / (float)time;
                    //sprintf(status, "time: %ld, fps:%5.1f avg fps:%5.1f", time, fps, averagefps);
                    sprintf(status, "frame: %ld", vid->frameCount);
#ifdef __APPLE__
                    CGContextRef ctx;
                    QDBeginCGContext(vid->pGWorld, &ctx);
                    CFStringRef str = CFStringCreateWithCString(NULL, status, kCFStringEncodingMacRoman);
                    CGContextSelectFont(ctx, "Monaco", 12, kCGEncodingMacRoman);
                    CGContextSetTextDrawingMode(ctx, kCGTextFillStroke);
                    CGContextShowTextAtPoint(ctx, 10, 10, status, strlen(status));
                    CFRelease(str);
                    QDEndCGContext(vid->pGWorld, &ctx);
#else
                    Str255 theString;
                    CGrafPtr theSavedPort;
                    GDHandle theSavedDevice;
                    GetGWorld(&theSavedPort, &theSavedDevice);
                    SetGWorld(vid->pGWorld, NULL);
                    TextSize(12);
                    TextMode(srcCopy);
                    MoveTo(vid->theRect.left + 10, vid->theRect.bottom - 14);
                    CopyCStringToPascal(status, theString);
                    DrawString(theString);
                    SetGWorld(theSavedPort, theSavedDevice);
#endif
                    //vid->lastTime = time;
                }
                // Mark status to indicate we have a frame available.
                vid->status |= AR_VIDEO_QUICKTIME_STATUS_BIT_READY;			
            }
            
            // All done. Welease Wodger!
            ar2VideoInternalUnlock(&(vid->bufMutex));
            usleep(vid->milliSecPerTimer * 1000);
            if (!ar2VideoInternalLock(&(vid->bufMutex))) {
                ARLOGe("ar2VideoInternalThread(): Unable to lock mutex, exiting.\n");
                break;
            }
            
            pthread_testcancel();
        }
    }
		
	pthread_cleanup_pop(1);
	return (NULL);
}

#ifdef __APPLE__
static int sysctlbyname_with_pid (const char *name, pid_t pid,
								  void *oldp, size_t *oldlenp,
								  void *newp, size_t newlen)
{
    if (pid == 0) {
        if (sysctlbyname(name, oldp, oldlenp, newp, newlen) == -1)  {
            ARLOGe("sysctlbyname_with_pid(0): sysctlbyname  failed:"
					"%s\n", strerror(errno));
            return -1;
        }
    } else {
        int mib[CTL_MAXNAME];
        size_t len = CTL_MAXNAME;
        if (sysctlnametomib(name, mib, &len) == -1) {
            ARLOGe("sysctlbyname_with_pid: sysctlnametomib  failed:"
					"%s\n", strerror(errno));
            return -1;
        }
        mib[len] = pid;
        len++;
        if (sysctl(mib, len, oldp, oldlenp, newp, newlen) == -1)  {
            ARLOGe("sysctlbyname_with_pid: sysctl  failed:"
                    "%s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}

// Pass 0 to use current PID.
static int is_pid_native (pid_t pid)
{
    int ret = 0;
    size_t sz = sizeof(ret);
	if (sysctlbyname_with_pid("sysctl.proc_native", pid,
							  &ret, &sz, NULL, 0) == -1) {
		if (errno == ENOENT) {
            // sysctl doesn't exist, which means that this version of Mac OS
            // pre-dates Rosetta, so the application must be native.
            return 1;
        }
        ARLOGe("is_pid_native: sysctlbyname_with_pid  failed:"
                "%s\n", strerror(errno));
        return -1;
    }
    return ret;
}
#endif // __APPLE__

#pragma mark -

int ar2VideoDispOptionQuickTime(void)
{
	//     0         1         2         3         4         5         6         7
	//     0123456789012345678901234567890123456789012345678901234567890123456789012
    ARLOG("ARVideo may be configured using one or more of the following options,\n");
    ARLOG("separated by a space:\n\n");
    ARLOG(" -[no]dialog\n");
    ARLOG("    Don't display video settings dialog.\n");
    ARLOG(" -[no]standarddialog\n");
    ARLOG("    Don't remove unnecessary panels from video settings dialog.\n");
    ARLOG(" -width=w\n");
    ARLOG("    Scale camera native image to width w.\n");
    ARLOG(" -height=h\n");
    ARLOG("    Scale camera native image to height h.\n");
    ARLOG(" -[no]fps\n");
    ARLOG("    Overlay camera frame counter on image.\n");
    ARLOG(" -grabber=n\n");
    ARLOG("    With multiple QuickTime video grabber components installed,\n");
	ARLOG("    use component n (default n=1).\n");
	ARLOG("    N.B. It is NOT necessary to use this option if you have installed\n");
	ARLOG("    more than one video input device (e.g. two cameras) as the default\n");
	ARLOG("    QuickTime grabber can manage multiple video channels.\n");
	ARLOG(" -pixelformat=cccc\n");
    ARLOG("    Return images with pixels in format cccc, where cccc is either a\n");
    ARLOG("    numeric pixel format number or a valid 4-character-code for a\n");
    ARLOG("    pixel format.\n");
	ARLOG("    The following numeric values are supported: \n");
	ARLOG("    24 (24-bit RGB), 32 (32-bit ARGB), 40 (8-bit grey)\n");
	ARLOG("    The following 4-character-codes are supported: \n");
    ARLOG("    BGRA, RGBA, ABGR, 24BG, 2vuy, yuvs.\n");
    ARLOG("    (See http://developer.apple.com/library/mac/#technotes/tn2010/tn2273.html.)\n");
    ARLOG(" -[no]fliph\n");
    ARLOG("    Flip camera image horizontally.\n");
    ARLOG(" -[no]flipv\n");
    ARLOG("    Flip camera image vertically.\n");
    ARLOG(" -[no]singlebuffer\n");
    ARLOG("    Use single buffering of captured video instead of triple-buffering.\n");
    ARLOG("\n");

    return (0);
}


AR2VideoParamQuickTimeT *ar2VideoOpenQuickTime(const char *config)
{
	int					itsAMovie = 0;
	long				qtVersion = 0L;
	int					width = 0;
	int					height = 0;
	int					grabber = 1;
	int					showFPS = 0;
	int					showDialog = 1;
	int					standardDialog = 0;
	int					singleBuffer = 0;
	int					flipH = 0, flipV = 0;
    OSErr				err_s = noErr;
	ComponentResult		err = noErr;
	int					err_i = 0;
    AR2VideoParamQuickTimeT		*vid = NULL;
    const char          *a;
    char                line[1024];
	char				movieConf[256] = "-offscreen -pause -loop";
	int					i;
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
	int					weLocked = 0;
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
	OSType				pixFormat = (OSType)0;
	long				bytesPerPixel;
#ifdef __APPLE__
	long				cpuType;
#endif
	AR_VIDEO_QUICKTIME_MOVIE_t movie;
	

	// Process configuration options.
    if (config) {
		a = config;
		err_i = 0;
        for (;;) {

            if (itsAMovie) {
				if (!gMoviesActiveCount) {
					if ((err_i = arVideoQuickTimeMovieInit()) != AR_E_NONE) {
						ARLOGe("ar2VideoOpenQuickTime(): Unable to initialise QuickTime for movie playback (%d).\n", err_i);
						return (NULL);
					}
				}
				strncat(movieConf, a, sizeof(movieConf) - strlen(movieConf) - 1);
				if ((err_i = arVideoQuickTimeMoviePlay(line, movieConf, &movie)) != AR_E_NONE) {
					ARLOGe("ar2VideoOpenQuickTime(): Unable to open QuickTime movie \"%s\" for playback (%d).\n", line, err_i);
					return (NULL);
				}
				if ((err_i = arVideoQuickTimeMovieIdle(movie)) != AR_E_NONE) {
					ARLOGe("ar2VideoOpenQuickTime(): Unable to service QuickTime movie \"%s\" (%d).\n", line, err_i);
					return (NULL);
				}
				gMoviesActiveCount++;
				// Allocate the parameters structure and fill it in.
				arMallocClear(vid, AR2VideoParamQuickTimeT, 1);
				vid->itsAMovie = 1;
				vid->movie = movie;
				return (vid);
				
			} else {
				
				while (*a == ' ' || *a == '\t') a++; // Skip whitespace.
				if (*a == '\0') break;
				
				if (strncmp(a, "-device=QUICKTIME", 17) == 0) {
				} else if (strncmp(a, "-movie=", 7) == 0) {
					// Attempt to read in movie pathname or URL, allowing for quoting of whitespace.
					a += 7; // Skip "-movie=" characters.
					if (*a == '"') {
						a++;
						// Read all characters up to next '"'.
						i = 0;
						while (i < (sizeof(line) - 1) && *a != '\0') {
							line[i] = *a;
							a++;
							if (line[i] == '"') break;
							i++;
						}
						line[i] = '\0';
					} else {
						sscanf(a, "%s", line);
					}
					if (!strlen(line)) err_i = 1;
					else itsAMovie = 1;
				} else if (strncmp(a, "-width=", 7) == 0) {
					sscanf(a, "%s", line);
					if (strlen(line) <= 7 || sscanf(&line[7], "%d", &width) == 0) err_i = 1;
				} else if (strncmp(a, "-height=", 8) == 0) {
					sscanf(a, "%s", line);
					if (strlen(line) <= 8 || sscanf(&line[8], "%d", &height) == 0) err_i = 1;
				} else if (strncmp(a, "-grabber=", 9) == 0) {
					sscanf(a, "%s", line);
					if (strlen(line) <= 9 || sscanf(&line[9], "%d", &grabber) == 0) err_i = 1;
				} else if (strncmp(a, "-pixelformat=", 13) == 0) {
					sscanf(a, "%s", line);
					if (strlen(line) <= 13) err_i = 1;
					else {
#ifdef AR_BIG_ENDIAN
						if (strlen(line) == 17) err_i = (sscanf(&line[13], "%4c", (char *)&pixFormat) < 1);
#else
						if (strlen(line) == 17) err_i = (sscanf(&line[13], "%c%c%c%c", &(((char *)&pixFormat)[3]), &(((char *)&pixFormat)[2]), &(((char *)&pixFormat)[1]), &(((char *)&pixFormat)[0])) < 4);
#endif
						else err_i = (sscanf(&line[13], "%li", (long *)&pixFormat) < 1); // Integer.
					}
				} else if (strncmp(a, "-fps", 4) == 0) {
					showFPS = 1;
				} else if (strncmp(a, "-nofps", 6) == 0) {
					showFPS = 0;
				} else if (strncmp(a, "-dialog", 7) == 0) {
					showDialog = 1;
				} else if (strncmp(a, "-nodialog", 9) == 0) {
					showDialog = 0;
				} else if (strncmp(a, "-standarddialog", 15) == 0) {
					standardDialog = 1;
				} else if (strncmp(a, "-nostandarddialog", 17) == 0) {
					standardDialog = 0;
				} else if (strncmp(a, "-fliph", 6) == 0) {
					flipH = 1;
				} else if (strncmp(a, "-nofliph", 8) == 0) {
					flipH = 0;
				} else if (strncmp(a, "-flipv", 6) == 0) {
					flipV = 1;
				} else if (strncmp(a, "-noflipv", 8) == 0) {
					flipV = 0;
				} else if (strncmp(a, "-singlebuffer", 13) == 0) {
					singleBuffer = 1;
				} else if (strncmp(a, "-nosinglebuffer", 15) == 0) {
					singleBuffer = 0;
				} else {
					err_i = 1;
				}
				
				if (err_i) {
					ARLOGe("Error: unrecognised video configuration option \"%s\".\n", a);
					ar2VideoDispOptionQuickTime();
					return (NULL);
				}
				while (*a != ' ' && *a != '\t' && *a != '\0') a++; // Skip to next whitespace.
			}
        }
    }
	
	// If no pixel format was specified in command-line options,
	// assign the one specified at compile-time as the default.
	if (!pixFormat) {
        switch (AR_DEFAULT_PIXEL_FORMAT) {
            case AR_PIXEL_FORMAT_2vuy:
                pixFormat = k2vuyPixelFormat;		// k422YpCbCr8CodecType, k422YpCbCr8PixelFormat
                break;
            case AR_PIXEL_FORMAT_yuvs:
                pixFormat = kYUVSPixelFormat;		// kComponentVideoUnsigned
                break;
            case AR_PIXEL_FORMAT_RGB:
                pixFormat = k24RGBPixelFormat;
                break;
            case AR_PIXEL_FORMAT_BGR:
                pixFormat = k24BGRPixelFormat;
                break;
            case AR_PIXEL_FORMAT_ARGB:
                pixFormat = k32ARGBPixelFormat;
                break;
            case AR_PIXEL_FORMAT_RGBA:
                pixFormat = k32RGBAPixelFormat;
                break;
            case AR_PIXEL_FORMAT_ABGR:
                pixFormat = k32ABGRPixelFormat;
                break;
            case AR_PIXEL_FORMAT_BGRA:
                pixFormat = k32BGRAPixelFormat;
                break;
            case AR_PIXEL_FORMAT_MONO:
                pixFormat = k8IndexedGrayPixelFormat;
                break;
            default:
                ARLOGe("Error: Unsupported default pixel format specified.\n");
                return (NULL);
        }
	}
	
	switch (pixFormat) {
		case k2vuyPixelFormat:
		case kYUVSPixelFormat:
			bytesPerPixel = 2l;
			break; 
		case k24RGBPixelFormat:
		case k24BGRPixelFormat:
			bytesPerPixel = 3l;
			break;
		case k32ARGBPixelFormat:
		case k32BGRAPixelFormat:
		case k32ABGRPixelFormat:
		case k32RGBAPixelFormat:
			bytesPerPixel = 4l;
			break;
		case k8IndexedGrayPixelFormat:
			bytesPerPixel = 1l;
			break;
		default:
#ifdef AR_BIG_ENDIAN
			ARLOGe("ar2VideoOpenQuickTime(): Unsupported pixel format requested:  0x%08x = %u = '%c%c%c%c'.\n", (unsigned int)pixFormat, (unsigned int)pixFormat, ((char *)&pixFormat)[0], ((char *)&pixFormat)[1], ((char *)&pixFormat)[2], ((char *)&pixFormat)[3]);
#else
			ARLOGe("ar2VideoOpenQuickTime(): Unsupported pixel format requested:  0x%08x = %u = '%c%c%c%c'.\n", (unsigned int)pixFormat, (unsigned int)pixFormat, ((char *)&pixFormat)[3], ((char *)&pixFormat)[2], ((char *)&pixFormat)[1], ((char *)&pixFormat)[0]);
#endif
			return(NULL);
			break;			
	}
	
	// If there are no active grabbers, init QuickTime.
	if (gGrabberActiveCount == 0) {
	
#ifdef _WIN32
		if ((err_s = InitializeQTML(0)) != noErr) {
			ARLOGe("ar2VideoOpenQuickTime(): OS error: QuickTime not installed.\n");
			return (NULL);
		}
#endif // _WIN32

		if ((err_s = Gestalt(gestaltQuickTimeVersion, &qtVersion)) != noErr) {
			ARLOGe("ar2VideoOpenQuickTime(): OS error: QuickTime not installed (%d).\n", err_s);
			return (NULL);
		}
		
#ifndef AR_VIDEO_SUPPORT_OLD_QUICKTIME
		if ((qtVersion >> 16) < 0x640) {
			ARLOGe("ar2VideoOpenQuickTime(): QuickTime version 6.4 or newer is required by this program.\n");;
			return (NULL);
		}
#else
		if ((qtVersion >> 16) < 0x400) {
			ARLOGe("ar2VideoOpenQuickTime(): QuickTime version 4.0 or newer is required by this program.\n");;
			return (NULL);
		}
#endif // !AR_VIDEO_SUPPORT_OLD_QUICKTIME
		
		// Initialise QuickTime (a.k.a. Movie Toolbox).
		if ((err_s = EnterMovies()) != noErr) {
			ARLOGe("ar2VideoOpenQuickTime(): Unable to initialise QuickTime (%d).\n", err_s);
			return (NULL);
		}
		
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
		// Init the QuickTime access mutex.		
		if ((err_i = pthread_mutex_init(&gGrabberQuickTimeMutex, NULL)) != 0) {
			ARLOGe("ar2VideoOpenQuickTime(): Error %d creating mutex (for QuickTime).\n", err_i);
			return (NULL);
		}
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
	}
	
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
	// Get a hold on the QuickTime toolbox.
	// Need to unlock this mutex before returning, so any errors should goto bail;
	if (gGrabberActiveCount > 0) {
		if (!ar2VideoInternalLock(&gGrabberQuickTimeMutex)) {
			ARLOGe("ar2VideoOpen(): Unable to lock mutex (for QuickTime).\n");
			return (NULL);
		}
		weLocked = 1;
	}
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
		
	gGrabberActiveCount++;
	
	// Allocate the parameters structure and fill it in.
	arMallocClear(vid, AR2VideoParamQuickTimeT, 1);
    vid->status         = 0;
	vid->showFPS		= showFPS;
	vid->frameCount		= 0;
	//vid->lastTime		= 0;
	//vid->timeScale	= 0;
	vid->grabber		= grabber;
	vid->bufCopyFlag	= !singleBuffer;
	vid->pixFormat		= pixFormat;
	
#ifdef __APPLE__
	// Find out if we are running on an Intel Mac.
	if ((err_s = Gestalt(gestaltNativeCPUtype, &cpuType) != noErr)) {
		ARLOGe("ar2VideoOpen(): Error getting native CPU type.\n");
		goto out1;
	}
	if (cpuType == gestaltCPUPentium) {
		// We are running native on an Intel-based Mac.
		//ARLOGd("Detected Intel CPU.\n");
	} else {
		int native = is_pid_native(0);
		// We are not. But are we running under Rosetta?
		if (native == 0) {
			// We're running under Rosetta.
			ARLOGw("Detected Intel CPU, but running PowerPC code under Rosetta.\n");
		} else if (native == 1) {
			//ARLOGd("Detected PowerPC CPU.\n");
		} else {
			// Error.
		}
	}
#endif // __APPLE__

	if(!(vid->pVdg = vdgAllocAndInit(grabber))) {
		ARLOGe("ar2VideoOpen(): vdgAllocAndInit returned error.\n");
		goto out1;
	}
	
	if ((err = vdgRequestSettings(vid->pVdg, showDialog, standardDialog, gGrabberActiveCount))) {
		ARLOGe("ar2VideoOpen(): vdgRequestSettings err=%ld.\n", err);
		goto out2;
	}
	
	if ((err = vdgPreflightGrabbing(vid->pVdg))) {
		ARLOGe("ar2VideoOpen(): vdgPreflightGrabbing err=%ld.\n", err);
		goto out2;
	}
	
	// Set the timer frequency from the Vdig's Data Rate
	// ASC soft vdig fails this call
	if ((err = vdgGetDataRate(   vid->pVdg, 
								&vid->milliSecPerFrame,
								&vid->frameRate,
								&vid->bytesPerSecond))) {
		ARLOGe("ar2VideoOpen(): vdgGetDataRate err=%ld.\n", err);
		//goto out2; 
	}
	if (err == noErr) {
		// Some vdigs do return the frameRate but not the milliSecPerFrame
		if (vid->milliSecPerFrame == 0) {
            if (vid->frameRate != 0) vid->milliSecPerFrame = (1000L << 16) / vid->frameRate;
            else vid->milliSecPerFrame = 40; // Aim for 25 fps.
		} 
	}
	
	// Poll the vdig at twice the frame rate or between sensible limits
	vid->milliSecPerTimer = vid->milliSecPerFrame / 2;
	if (vid->milliSecPerTimer <= 0) {
		ARLOGe("vid->milliSecPerFrame: %ld ", vid->milliSecPerFrame);
		vid->milliSecPerTimer = AR_VIDEO_QUICKTIME_IDLE_INTERVAL_MILLISECONDS_MIN;
		ARLOGe("forcing timer period to %ldms\n", vid->milliSecPerTimer);
	}
	if (vid->milliSecPerTimer >= AR_VIDEO_QUICKTIME_IDLE_INTERVAL_MILLISECONDS_MAX) {
		ARLOGe("vid->milliSecPerFrame: %ld ", vid->milliSecPerFrame);
		vid->milliSecPerTimer = AR_VIDEO_QUICKTIME_IDLE_INTERVAL_MILLISECONDS_MAX;
		ARLOGe("forcing timer period to %ldms\n", vid->milliSecPerTimer);
	}
	
    vid->vdImageDesc = (ImageDescriptionHandle)NewHandle(0);
	if ((err = vdgGetImageDescription(vid->pVdg, vid->vdImageDesc))) {
		ARLOGe("ar2VideoOpen(): vdgGetImageDescription err=%ld\n", err);
		goto out3;
	}
	
	// Report video size and compression type.
	ARLOGi("Video cType is %c%c%c%c, size is %dx%d.\n",
			(char)(((*(vid->vdImageDesc))->cType >> 24) & 0xFF),
			(char)(((*(vid->vdImageDesc))->cType >> 16) & 0xFF),
			(char)(((*(vid->vdImageDesc))->cType >>  8) & 0xFF),
			(char)(((*(vid->vdImageDesc))->cType >>  0) & 0xFF),
			((*vid->vdImageDesc)->width), ((*vid->vdImageDesc)->height));
	
	// If a particular size was requested, set the size of the GWorld to
	// the request, otherwise set it to the size of the incoming video.
	vid->width = (width ? width : (int)((*vid->vdImageDesc)->width));
	vid->height = (height ? height : (int)((*vid->vdImageDesc)->height));
	MacSetRect(&(vid->theRect), 0, 0, (short)vid->width, (short)vid->height);	

	// Make a scaling matrix for the sequence if size of incoming video differs from GWorld dimensions.
	vid->scaleMatrixPtr = NULL;
	int doSourceScale;
	if (vid->width != (int)((*vid->vdImageDesc)->width) || vid->height != (int)((*vid->vdImageDesc)->height)) {
		arMalloc(vid->scaleMatrixPtr, MatrixRecord, 1);
		SetIdentityMatrix(vid->scaleMatrixPtr);
		Fixed scaleX, scaleY;
		scaleX = FixRatio(vid->width, (*vid->vdImageDesc)->width);
		scaleY = FixRatio(vid->height, (*vid->vdImageDesc)->height);
		ScaleMatrix(vid->scaleMatrixPtr, scaleX, scaleY, 0, 0);
		ARLOGi("Video will be scaled to size %dx%d.\n", vid->width, vid->height);
		doSourceScale = 1;
	} else {
		doSourceScale = 0;
	}
	
	// If a flip was requested, add a scaling matrix for it.
	if (flipH || flipV) {
		Fixed scaleX, scaleY;
		if (flipH) scaleX = -fixed1;
		else scaleX = fixed1;
		if (flipV) scaleY = -fixed1;
		else scaleY = fixed1;
		if (!doSourceScale) {
			arMalloc(vid->scaleMatrixPtr, MatrixRecord, 1);
			SetIdentityMatrix(vid->scaleMatrixPtr);
		}
		ScaleMatrix(vid->scaleMatrixPtr, scaleX, scaleY, FloatToFixed((float)(vid->width) * 0.5f), FloatToFixed((float)(vid->height) * 0.5f));
	}

	// Allocate buffer for the grabber to write pixel data into, and use
	// QTNewGWorldFromPtr() to wrap an offscreen GWorld structure around
	// it. We do it in these two steps rather than using QTNewGWorld()
	// to guarantee that we don't get padding bytes at the end of rows.
	vid->rowBytes = vid->width * bytesPerPixel;
	vid->bufSize = vid->height * vid->rowBytes;
	if (!(vid->bufPixels = (ARUint8 *)valloc(vid->bufSize * sizeof(ARUint8)))) exit (1);
	if (vid->bufCopyFlag) {
		// And another two buffers for OpenGL to read out of.
		if (!(vid->bufPixelsCopy1 = (ARUint8 *)valloc(vid->bufSize * sizeof(ARUint8)))) exit (1);
		if (!(vid->bufPixelsCopy2 = (ARUint8 *)valloc(vid->bufSize * sizeof(ARUint8)))) exit (1);
	}
	// Wrap a GWorld around the pixel buffer.
	err_s = QTNewGWorldFromPtr(&(vid->pGWorld),			// returned GWorld
							   vid->pixFormat,				// format of pixels
							   &(vid->theRect),			// bounds
							   0,						// color table
							   NULL,					// GDHandle
							   0,						// flags
							   (void *)(vid->bufPixels), // pixel base addr
							   vid->rowBytes);			// bytes per row
	if (err_s != noErr) {
		ARLOGe("ar2VideoOpen(): Unable to create offscreen buffer for sequence grabbing (%d).\n", err_s);
		goto out5;
	}
	
	// Lock the pixmap and make sure it's locked because
	// we can't decompress into an unlocked PixMap, 
	// and open the default sequence grabber.
	err_i = (int)LockPixels(GetGWorldPixMap(vid->pGWorld));
	if (!err_i) {
		ARLOGe("ar2VideoOpen(): Unable to lock buffer for sequence grabbing.\n");
		goto out6;
	}
	
	// Erase to black.
#ifdef __APPLE__
	CGContextRef ctx;
	QDBeginCGContext(vid->pGWorld, &ctx);
	CGContextSetRGBFillColor(ctx, 0, 0, 0, 1);               
	CGContextFillRect(ctx, CGRectMake(0, 0, (vid->theRect).left - (vid->theRect).right, (vid->theRect).top - (vid->theRect).bottom));
	CGContextFlush(ctx);
	QDEndCGContext (vid->pGWorld, &ctx);
#else
	CGrafPtr			theSavedPort;
	GDHandle			theSavedDevice;
    GetGWorld(&theSavedPort, &theSavedDevice);    
    SetGWorld(vid->pGWorld, NULL);
    BackColor(blackColor);
    ForeColor(whiteColor);
    EraseRect(&(vid->theRect));
    SetGWorld(theSavedPort, theSavedDevice);
#endif
	
	// Set the decompression destination to the offscreen GWorld.
	if ((err_s = vdgSetDestination(vid->pVdg, vid->pGWorld))) {
		ARLOGe("ar2VideoOpen(): vdgSetDestination err=%d\n", err_s);
		goto out6;	
	}
	
	// Initialise per-vid pthread variables.
	// Create a mutex to protect access to data structures.
	if ((err_i = pthread_mutex_init(&(vid->bufMutex), NULL)) != 0) {
		ARLOGe("ar2VideoOpen(): Error %d creating mutex.\n", err_i);
	}
	// Create condition variable.
	if (err_i == 0) {
		if ((err_i = pthread_cond_init(&(vid->condition), NULL)) != 0) {
			ARLOGe("ar2VideoOpen(): Error %d creating condition variable.\n", err_i);
			pthread_mutex_destroy(&(vid->bufMutex));
		}
	}
		
	if (err_i != 0) { // Clean up component on failure to init per-vid pthread variables.
		goto out6;
	}

	goto out;
	
out6:
	DisposeGWorld(vid->pGWorld);
out5:
	if (vid->bufCopyFlag) {
		free(vid->bufPixelsCopy2);
		free(vid->bufPixelsCopy1);
	}
	free(vid->bufPixels);
	if (vid->scaleMatrixPtr) free(vid->scaleMatrixPtr);
out3:
	DisposeHandle((Handle)vid->vdImageDesc);
out2:	
	vdgReleaseAndDealloc(vid->pVdg);
out1:	
	free(vid);
	vid = NULL;
	gGrabberActiveCount--;
out:
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
	// Release our hold on the QuickTime toolbox.
	if (weLocked) {
		if (!ar2VideoInternalUnlock(&gGrabberQuickTimeMutex)) {
			ARLOGe("ar2VideoOpen(): Unable to unlock mutex (for QuickTime).\n");
		}
	}
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
	
	return (vid);
}

int ar2VideoCloseQuickTime(AR2VideoParamQuickTimeT *vid)
{
	int err_i;
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
	int weLocked = 0;
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
	
    if( vid == NULL ) return -1;

	if (vid->itsAMovie) {
		arVideoQuickTimeMovieStop(&(vid->movie));
		free (vid);
		gMoviesActiveCount--;
		if (!gMoviesActiveCount) arVideoQuickTimeMovieFinal();
	} else {
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
		// Get a hold on the QuickTime toolbox.
		if (gGrabberActiveCount > 1) {
			if (!ar2VideoInternalLock(&gGrabberQuickTimeMutex)) {
				ARLOGe("ar2VideoClose(): Unable to lock mutex (for QuickTime).\n");
			}
			weLocked = 1;
		}
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
		
		// Destroy the condition variable.
		if ((err_i = pthread_cond_destroy(&(vid->condition))) != 0) {
			ARLOGe("ar2VideoClose(): Error %d destroying condition variable.\n", err_i);
		}
		
		// Destroy the mutex.
		if ((err_i = pthread_mutex_destroy(&(vid->bufMutex))) != 0) {
			ARLOGe("ar2VideoClose(): Error %d destroying mutex.\n", err_i);
		}
	    
		if (vid->pGWorld != NULL) {
			DisposeGWorld(vid->pGWorld);
			vid->pGWorld = NULL;
		}
		
		if (vid->bufCopyFlag) {
			if (vid->bufPixelsCopy2) {
				free(vid->bufPixelsCopy2);
				vid->bufPixelsCopy2 = NULL;
			}
			if (vid->bufPixelsCopy1) {
				free(vid->bufPixelsCopy1);
				vid->bufPixelsCopy1 = NULL;
			}
		}
		if (vid->bufPixels) {
			free(vid->bufPixels);
			vid->bufPixels = NULL;
		}
		
		if (vid->scaleMatrixPtr) {
			free(vid->scaleMatrixPtr);
			vid->scaleMatrixPtr = NULL;
		}
		
		if (vid->vdImageDesc) {
			DisposeHandle((Handle)vid->vdImageDesc);
			vid->vdImageDesc = NULL;
		}
		
		vdgReleaseAndDealloc(vid->pVdg);
		
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
		// Release our hold on the QuickTime toolbox.
		if (weLocked) {
			if (!ar2VideoInternalUnlock(&gGrabberQuickTimeMutex)) {
				ARLOGe("ar2VideoClose(): Unable to unlock mutex (for QuickTime).\n");
			}
		}
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
		
		// Count one less grabber running.
		free (vid);
		gGrabberActiveCount--;
		
		// If we're the last to close, clean up after everybody.
		if (!gGrabberActiveCount) {
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
			// Destroy the mutex.
			if ((err_i = pthread_mutex_destroy(&gGrabberQuickTimeMutex)) != 0) {
				ARLOGe("ar2VideoClose(): Error %d destroying mutex (for QuickTime).\n", err_i);
			}
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
			
			// Probably a good idea to close down QuickTime.
			ExitMovies();
#ifdef _WIN32	
			TerminateQTML();
#endif // _WIN32	
		}
	}
	
    return (0);
}

int ar2VideoCapStartQuickTime(AR2VideoParamQuickTimeT *vid)
{
	ComponentResult err;
	int err_i = 0;
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
	int weLocked = 0;
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
	
    if( vid == NULL ) return -1;

	if (vid->itsAMovie) {
		err_i = arVideoQuickTimeMovieSetPlayRate(vid->movie, 1.0f);
	} else {
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
		// Get a hold on the QuickTime toolbox.
		if (gGrabberActiveCount > 1) {
			if (!ar2VideoInternalLock(&gGrabberQuickTimeMutex)) {
				ARLOGe("ar2VideoCapStart(): Unable to lock mutex (for QuickTime).\n");
				return (-1);
			}
			weLocked = 1;
		}
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
		
		vid->status = 0;
		if (!vid->pVdg->isPreflighted) {
			if ((err = vdgPreflightGrabbing(vid->pVdg))) {
				ARLOGe("ar2VideoCapStart(): vdgPreflightGrabbing err=%ld\n", err);
				err_i = (int)err;
			}		
		}
		
		if (err_i == 0) {
			if ((err = vdgStartGrabbing(vid->pVdg, vid->scaleMatrixPtr))) {
				ARLOGe("ar2VideoCapStart(): vdgStartGrabbing err=%ld\n", err);
				err_i = (int)err;
			}
		}
		
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
		// Release our hold on the QuickTime toolbox.
		if (weLocked) {
			if (!ar2VideoInternalUnlock(&gGrabberQuickTimeMutex)) {
				ARLOGe("ar2VideoCapStart(): Unable to unlock mutex (for QuickTime).\n");
				return (-1);
			}
		}
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
		
		if (err_i == 0) {
			// Create the new thread - no attr, vid as user data.
			vid->threadRunning = 1;
			if ((err_i = pthread_create(&(vid->thread), NULL, ar2VideoInternalThread, (void *)vid)) != 0) {
				vid->threadRunning = 0;
				ARLOGe("ar2VideoCapStart(): Error %d detaching thread.\n", err_i);
			}
		}
	}

	return (err_i);
}

int ar2VideoCapStopQuickTime(AR2VideoParamQuickTimeT *vid)
{
	int err_i = 0;
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
	int weLocked = 0;
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
	void *exit_status_p; // Pointer to return value from thread, will be filled in by pthread_join().
	ComponentResult err = noErr;
	
    if( vid == NULL ) return -1;

	if (vid->itsAMovie) {
		err_i = arVideoQuickTimeMovieSetPlayRate(vid->movie, 0.0f);
	} else {
		if (vid->threadRunning) {
			// Cancel thread.
			if ((err_i = pthread_cancel(vid->thread)) != 0) {
				ARLOGe("ar2VideoCapStop(): Error %d cancelling ar2VideoInternalThread().\n", err_i);
				return (err_i);
			}
			
			// Wait for join.
			if ((err_i = pthread_join(vid->thread, &exit_status_p)) != 0) {
				ARLOGe("ar2VideoCapStop(): Error %d waiting for ar2VideoInternalThread() to finish.\n", err_i);
				return (err_i);
			}
			vid->threadRunning = 0;
			
			// Exit status is ((exit_status_p == AR_PTHREAD_CANCELLED) ? 0 : *(ERROR_t *)(exit_status_p))
		}
		
		if (vid->pVdg) {
			
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
			// Get a hold on the QuickTime toolbox.
			if (gGrabberActiveCount > 1) {
				if (!ar2VideoInternalLock(&gGrabberQuickTimeMutex)) {
					ARLOGe("ar2VideoCapStop(): Unable to lock mutex (for QuickTime).\n");
					return (-1);
				}
				weLocked = 1;
			}
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
			
			if ((err = vdgStopGrabbing(vid->pVdg)) != noErr) {
				ARLOGe("vdgStopGrabbing err=%ld\n", err);
				err_i = (int)err;
			}
			vid->status = 0;
			vid->pVdg->isPreflighted = 0;
			
#ifdef AR_VIDEO_SUPPORT_OLD_QUICKTIME
			// Release our hold on the QuickTime toolbox.
			if (weLocked) {
				if (!ar2VideoInternalUnlock(&gGrabberQuickTimeMutex)) {
					ARLOGe("ar2VideoCapStop(): Unable to unlock mutex (for QuickTime).\n");
					return (-1);
				}
			}
#endif // AR_VIDEO_SUPPORT_OLD_QUICKTIME
			
		}
	}
	
	return (err_i);
}

int ar2VideoGetIdQuickTime( AR2VideoParamQuickTimeT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    if( vid == NULL ) return -1;
	
    *id0 = 0;
    *id1 = 0;
	
    return -1;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatQuickTime( AR2VideoParamQuickTimeT *vid )
{
	AR_PIXEL_FORMAT pixelFormat;
	
    if( vid == NULL ) return AR_PIXEL_FORMAT_INVALID;
	
	if (vid->itsAMovie) {
		arVideoQuickTimeMovieGetFrameSize(vid->movie, NULL, NULL, NULL, (unsigned int *)&pixelFormat);
	} else {
		// Need lock to guarantee exclusive access to vid.
		if (!ar2VideoInternalLock(&(vid->bufMutex))) {
			ARLOGe("ar2VideoInqSize(): Unable to lock mutex.\n");
			return (AR_PIXEL_FORMAT_INVALID);
		}
		
		switch (vid->pixFormat) {
			case k2vuyPixelFormat:		// k422YpCbCr8CodecType, k422YpCbCr8PixelFormat
				pixelFormat = AR_PIXEL_FORMAT_2vuy;
				break;
			case kYUVSPixelFormat:		// kComponentVideoUnsigned
				pixelFormat = AR_PIXEL_FORMAT_yuvs;
				break;		
			case k24RGBPixelFormat:
				pixelFormat = AR_PIXEL_FORMAT_RGB;
				break;
			case k24BGRPixelFormat:
				pixelFormat = AR_PIXEL_FORMAT_BGR;
				break;
			case k32ARGBPixelFormat:
				pixelFormat = AR_PIXEL_FORMAT_ARGB;
				break;
			case k32RGBAPixelFormat:
				pixelFormat = AR_PIXEL_FORMAT_RGBA;
				break;
			case k32ABGRPixelFormat:
				pixelFormat = AR_PIXEL_FORMAT_ABGR;
				break;
			case k32BGRAPixelFormat:
				pixelFormat = AR_PIXEL_FORMAT_BGRA;
				break;
			case k8IndexedGrayPixelFormat:
				pixelFormat = AR_PIXEL_FORMAT_MONO;
				break;
			default:
				pixelFormat = AR_PIXEL_FORMAT_INVALID;
		}
		
		if (!ar2VideoInternalUnlock(&(vid->bufMutex))) {
			ARLOGe("ar2VideoInqSize(): Unable to unlock mutex.\n");
			return (AR_PIXEL_FORMAT_INVALID);
		}
	}

    return (pixelFormat);
}

int ar2VideoGetSizeQuickTime(AR2VideoParamQuickTimeT *vid, int *x,int *y)
{
    if( vid == NULL ) return -1;

	if (vid->itsAMovie) {
		arVideoQuickTimeMovieGetFrameSize(vid->movie, (unsigned int *)x, (unsigned int *)y, NULL, NULL);
	} else {
		// Need lock to guarantee exclusive access to vid.
		if (!ar2VideoInternalLock(&(vid->bufMutex))) {
			ARLOGe("ar2VideoInqSize(): Unable to lock mutex.\n");
			return (-1);
		}
		
		*x = vid->width;
		*y = vid->height;

		if (!ar2VideoInternalUnlock(&(vid->bufMutex))) {
			ARLOGe("ar2VideoInqSize(): Unable to unlock mutex.\n");
			return (-1);
		}
	}
    return (0);
}

AR2VideoBufferT *ar2VideoGetImageQuickTime(AR2VideoParamQuickTimeT *vid)
{
	int err_i;
	
    if (vid == NULL) return (NULL);

	if (vid->itsAMovie) {
        if (arVideoQuickTimeMovieIsPlayingEveryFrame(vid->movie)) {
            if ((err_i = arVideoQuickTimeMovieSetPlayPositionNextFrame(vid->movie)) == AR_E_EOF) {
                // Here is where we would signal the caller that the movie has ended.
                return (NULL);
            }
        }
		if ((err_i = arVideoQuickTimeMovieIdle(vid->movie)) == AR_E_EOF) {
			// Here is where we would signal the caller that the movie has ended.
			return (NULL);
		} else if (err_i != AR_E_NONE) {
			return (NULL);
		} else {
            double time;
			vid->arVideoBuffer.buff = arVideoQuickTimeMovieGetFrame(vid->movie, &time);
            if ((vid->arVideoBuffer).buff) {
                vid->arVideoBuffer.fillFlag = 1;
                vid->arVideoBuffer.time_sec = (ARUint32)time;
                vid->arVideoBuffer.time_usec = (ARUint32)((time - (double)vid->arVideoBuffer.time_sec)*1E6);
            } else {
                vid->arVideoBuffer.fillFlag = 0;
            }
			return (&(vid->arVideoBuffer));
		}
	} else {
		// ar2VideoGetImage() used to block waiting for a frame.
		// This locked the OpenGL frame rate to the camera frame rate.
		// Now, if no frame is currently available then we won't wait around for one.
		// So, do we have a new frame from the sequence grabber?	
		if (vid->status & AR_VIDEO_QUICKTIME_STATUS_BIT_READY) {
			
			//ARLOGe("For vid @ %p got frame %ld.\n", vid, vid->frameCount);
			// If triple-buffering, time to copy buffer.
			if (vid->bufCopyFlag) {
				// Need lock to guarantee this thread exclusive access to vid.
				if (!ar2VideoInternalLock(&(vid->bufMutex))) {
					ARLOGe("ar2VideoGetImage(): Unable to lock mutex.\n");
					return (NULL);
				}
				if (vid->status & AR_VIDEO_QUICKTIME_STATUS_BIT_BUFFER) {
					memcpy((void *)(vid->bufPixelsCopy2), (void *)(vid->bufPixels), vid->bufSize);
					(vid->arVideoBuffer).buff = vid->bufPixelsCopy2;
					vid->status &= ~AR_VIDEO_QUICKTIME_STATUS_BIT_BUFFER; // Clear buffer bit.
				} else {
					memcpy((void *)(vid->bufPixelsCopy1), (void *)(vid->bufPixels), vid->bufSize);
					(vid->arVideoBuffer).buff = vid->bufPixelsCopy1;
					vid->status |= AR_VIDEO_QUICKTIME_STATUS_BIT_BUFFER; // Set buffer bit.
				}
				if (!ar2VideoInternalUnlock(&(vid->bufMutex))) {
					ARLOGe("ar2VideoGetImage(): Unable to unlock mutex.\n");
					return (NULL);
				}
			} else {
				(vid->arVideoBuffer).buff = vid->bufPixels;
			}
			
			vid->status &= ~AR_VIDEO_QUICKTIME_STATUS_BIT_READY; // Clear ready bit.
            (vid->arVideoBuffer).fillFlag = 1;
			return (&(vid->arVideoBuffer));
			
		} else {
			return (NULL);
		}
	}
}

int ar2VideoGetParamiQuickTime( AR2VideoParamQuickTimeT *vid, int paramName, int *value )
{
    return -1;
}
int ar2VideoSetParamiQuickTime( AR2VideoParamQuickTimeT *vid, int paramName, int  value )
{
    return -1;
}
int ar2VideoGetParamdQuickTime( AR2VideoParamQuickTimeT *vid, int paramName, double *value )
{
    return -1;
}
int ar2VideoSetParamdQuickTime( AR2VideoParamQuickTimeT *vid, int paramName, double  value )
{
    return -1;
}

AR_VIDEO_QUICKTIME_MOVIE_t ar2VideoGetMovieQuickTime( AR2VideoParamQuickTimeT *vid )
{
    if (vid == NULL) return (NULL);
	if (vid->itsAMovie) return (vid->movie);
    else return (NULL);
}

#endif // AR_INPUT_QUICKTIME
