/*
 *  ARMovie.cpp
 *  ARToolKit for Android
 *
 *  An NFT example with all ARToolKit setup performed in native code,
 *  and with drawing a movie on the surface.
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not use, install, modify or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive
 *  license, under Daqri's copyrights in this original Daqri software (the
 *  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
 *  Software, with or without modifications, in source and/or binary forms;
 *  provided that if you redistribute the Daqri Software in its entirety and
 *  without modifications, you must retain this notice and the following
 *  text and disclaimers in all such redistributions of the Daqri Software.
 *  Neither the name, trademarks, service marks or logos of Daqri LLC may
 *  be used to endorse or promote products derived from the Daqri Software
 *  without specific prior written permission from Daqri.  Except as
 *  expressly stated in this notice, no other rights or licenses, express or
 *  implied, are granted by Daqri herein, including but not limited to any
 *  patent rights that may be infringed by your derivative works or by other
 *  works in which the Daqri Software may be incorporated.
 *
 *  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
 *  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 *  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
 *  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 *  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 *  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
 *  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 *  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Copyright 2015 Daqri LLC. All Rights Reserved.
 *  Copyright 2011-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Julian Looser, Philip Lamb
 */

// ============================================================================
//	Includes
// ============================================================================

#include <jni.h>
#include <android/log.h>
#include <stdlib.h> // malloc()
#include <pthread.h>

#include <AR/ar.h>
#include <AR/arMulti.h>
#include <AR/video.h>
#include <AR/gsub_es.h>
#include <AR/arFilterTransMat.h>
#include <AR2/tracking.h>

#include "ARMarkerNFT.h"
#include "trackingSub.h"

// ============================================================================
//	Types
// ============================================================================

typedef enum {
    ARViewContentModeScaleToFill,
    ARViewContentModeScaleAspectFit,      // contents scaled to fit with fixed aspect. remainder is transparent
    ARViewContentModeScaleAspectFill,     // contents scaled to fill with fixed aspect. some portion of content may be clipped.
    //ARViewContentModeRedraw,              // redraw on bounds change
    ARViewContentModeCenter,              // contents remain same size. positioned adjusted.
    ARViewContentModeTop,
    ARViewContentModeBottom,
    ARViewContentModeLeft,
    ARViewContentModeRight,
    ARViewContentModeTopLeft,
    ARViewContentModeTopRight,
    ARViewContentModeBottomLeft,
    ARViewContentModeBottomRight,
} ARViewContentMode;

enum viewPortIndices {
    viewPortIndexLeft = 0,
    viewPortIndexBottom,
    viewPortIndexWidth,
    viewPortIndexHeight
};

// ============================================================================
//	Constants
// ============================================================================

#define PAGES_MAX               10          // Maximum number of pages expected. You can change this down (to save memory) or up (to accomodate more pages.)

#ifndef MAX
#  define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#endif
#ifndef MIN
#  define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif

// Logging macros
#define  LOG_TAG    "ARMovieNative"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define CHECK_GL_ERROR() ({ GLenum __error = glGetError(); if(__error) LOGE("OpenGL error 0x%04X in %s\n", __error, __FUNCTION__); (__error ? 0 : 1); })

// ============================================================================
//	Function prototypes.
// ============================================================================

// Utility preprocessor directive so only one change needed if Java class name changes
#define JNIFUNCTION_NATIVE(sig) Java_org_artoolkit_ar_samples_ARMovie_ARMovieActivity_##sig

extern "C" {
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeCreate(JNIEnv* env, jobject object, jobject instanceOfAndroidContext));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeStart(JNIEnv* env, jobject object));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeStop(JNIEnv* env, jobject object));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeDestroy(JNIEnv* env, jobject object));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeVideoInit(JNIEnv* env, jobject object, jint w, jint h, jint cameraIndex, jboolean cameraIsFrontFacing));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeVideoFrame(JNIEnv* env, jobject obj, jbyteArray pinArray)) ;
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSurfaceCreated(JNIEnv* env, jobject object));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSurfaceChanged(JNIEnv* env, jobject object, jint w, jint h));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDisplayParametersChanged(JNIEnv* env, jobject object, jint orientation, jint width, jint height, jint dpi));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDrawFrame(JNIEnv* env, jobject obj, jint movieWidth, jint movieHeight, jint movieTextureID, jfloatArray movieTextureMtx));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeMovieInit(JNIEnv* env, jobject obj, jobject movieControllerThis, jobject movieControllerWeakThis)) ;
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeMovieFinal(JNIEnv* env, jobject obj)) ;
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSetInternetState(JNIEnv* env, jobject obj, jint state));
};

static void nativeVideoGetCparamCallback(const ARParam *cparam, void *userdata);
static void *loadNFTDataAsync(THREAD_HANDLE_T *threadHandle);
static int initNFT(ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat);

// ============================================================================
//	Global variables
// ============================================================================

// Preferences.
static const char *markerConfigDataFilename = "Data/markers.dat";

// Image acquisition.
static AR2VideoParamT *gVid = NULL;
static bool videoInited = false;                                    ///< true when ready to receive video frames.
static int videoWidth = 0;                                          ///< Width of the video frame in pixels.
static int videoHeight = 0;                                         ///< Height of the video frame in pixels.
static AR_PIXEL_FORMAT gPixFormat;                                  ///< Pixel format from ARToolKit enumeration.
static ARUint8* gVideoFrame = NULL;                                 ///< Buffer containing current video frame.
static size_t gVideoFrameSize = 0;                                  ///< Size of buffer containing current video frame.
static pthread_mutex_t gVideoFrameLock;
static bool videoFrameNeedsPixelBufferDataUpload = false;
static int gCameraIndex = 0;
static bool gCameraIsFrontFacing = false;

// Markers.
static ARMarkerNFT *markersNFT = NULL;
static int markersNFTCount = 0;

// NFT.
static THREAD_HANDLE_T     *trackingThreadHandle = NULL;
static AR2HandleT          *ar2Handle = NULL;
static KpmHandle           *kpmHandle = NULL;
static int                  surfaceSetCount = 0;
static AR2SurfaceSetT      *surfaceSet[PAGES_MAX];
static THREAD_HANDLE_T     *nftDataLoadingThreadHandle = NULL;
static int                  nftDataLoaded = false;

// NFT results.
static int detectedPage = -2; // -2 Tracking not inited, -1 tracking inited OK, >= 0 tracking online on page.
static float trackingTrans[3][4];

// Drawing.
static int backingWidth;
static int backingHeight;
static GLint viewPort[4];
static ARViewContentMode gContentMode = ARViewContentModeScaleAspectFill;
static bool gARViewLayoutRequired = false;
static ARParamLT *gCparamLT = NULL;                                 ///< Camera paramaters
static ARGL_CONTEXT_SETTINGS_REF gArglSettings = NULL;              ///< GL settings for rendering video background
static const ARdouble NEAR_PLANE = 10.0f;                           ///< Near plane distance for projection matrix calculation
static const ARdouble FAR_PLANE = 5000.0f;                          ///< Far plane distance for projection matrix calculation
static ARdouble cameraLens[16];
static ARdouble cameraPose[16];
static int cameraPoseValid;
static bool gARViewInited = false;

// Drawing orientation.
static int gDisplayOrientation = 1; // range [0-3]. 1=landscape.
static int gDisplayWidth = 0;
static int gDisplayHeight = 0;
static int gDisplayDPI = 160; // Android default.

static bool gContentRotate90 = false;
static bool gContentFlipV = false;
static bool gContentFlipH = false;

// Network.
static int gInternetState = -1;

// Movie control callbacks.
static jclass    mMovieJClass = NULL;
static jmethodID mMoviePlayJMethodID = NULL;
static jmethodID mMoviePauseJMethodID = NULL;
static jobject   mMovieJObjectWeak = NULL;

// ============================================================================
//	Functions
// ============================================================================

//
// Lifecycle functions.
// See http://developer.android.com/reference/android/app/Activity.html#ActivityLifecycle
//

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeCreate(JNIEnv* env, jobject object, jobject instanceOfAndroidContext))
{
    int err_i;
#ifdef DEBUG
    LOGI("nativeCreate\n");
#endif

    // Change working directory for the native process, so relative paths can be used for file access.
    arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST, NULL, instanceOfAndroidContext);

    // Load marker(s).
    newMarkers(markerConfigDataFilename, &markersNFT, &markersNFTCount);
    if (!markersNFTCount) {
        LOGE("Error loading markers from config. file '%s'.", markerConfigDataFilename);
        return false;
    }
#ifdef DEBUG
    LOGE("Marker count = %d\n", markersNFTCount);
#endif
    
	return (true);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeStart(JNIEnv* env, jobject object))
{
#ifdef DEBUG
    LOGI("nativeStart\n");
#endif

    gVid = ar2VideoOpen("");
    if (!gVid) {
    	LOGE("Error: ar2VideoOpen.\n");
    	return (false);
    }
    
    // Since most NFT init can't be completed until the video frame size is known,
    // and NFT surface loading depends on NFT init, all that will be deferred.
    
    // Also, VirtualEnvironment init depends on having an OpenGL context, and so that also 
    // forces us to defer VirtualEnvironment init.
    
    // ARGL init depends on both these things, which forces us to defer it until the
    // main frame loop.
        
	return (true);
}

// cleanup();
JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeStop(JNIEnv* env, jobject object))
{
#ifdef DEBUG
    LOGI("nativeStop\n");
#endif
    int i, j;
    
	// Can't call arglCleanup() here, because nativeStop is not called on rendering thread.

    // NFT cleanup.
    if (trackingThreadHandle) {
#ifdef DEBUG
        LOGI("Stopping NFT2 tracking thread.");
#endif
        trackingInitQuit(&trackingThreadHandle);
        detectedPage = -2;
    }
    j = 0;
    for (i = 0; i < surfaceSetCount; i++) {
        if (surfaceSet[i]) {
#ifdef DEBUG
            if (j == 0) LOGI("Unloading NFT tracking surfaces.");
#endif
            ar2FreeSurfaceSet(&surfaceSet[i]); // Sets surfaceSet[i] to NULL.
            j++;
        }
    }
#ifdef DEBUG
    if (j > 0) LOGI("Unloaded %d NFT tracking surfaces.", j);
#endif
    surfaceSetCount = 0;
    nftDataLoaded = false;
#ifdef DEBUG
	LOGI("Cleaning up ARToolKit NFT handles.");
#endif
    ar2DeleteHandle(&ar2Handle);
    kpmDeleteHandle(&kpmHandle);
    arParamLTFree(&gCparamLT);
    
    // OpenGL cleanup -- not done here.
    
    // Video cleanup.
    if (gVideoFrame) {
        free(gVideoFrame);
        gVideoFrame = NULL;
        gVideoFrameSize = 0;
        pthread_mutex_destroy(&gVideoFrameLock);
    }
    ar2VideoClose(gVid);
    gVid = NULL;
    videoInited = false;

    return (true);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeDestroy(JNIEnv* env, jobject object))
{
#ifdef DEBUG
    LOGI("nativeDestroy\n");
#endif
    if (markersNFT) deleteMarkers(&markersNFT, &markersNFTCount);
    
    return (true);
}

//
// Camera functions.
//

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeVideoInit(JNIEnv* env, jobject object, jint w, jint h, jint cameraIndex, jboolean cameraIsFrontFacing))
{
#ifdef DEBUG
    LOGI("nativeVideoInit\n");
#endif
	// As of ARToolKit v5.0, NV21 format video frames are handled natively,
	// and no longer require colour conversion to RGBA. A buffer (gVideoFrame)
	// must be set aside to copy the frame from the Java side.
	// If you still require RGBA format information from the video,
	// you can create your own additional buffer, and then unpack the NV21
	// frames into it in nativeVideoFrame() below.
	// Here is where you'd allocate the buffer:
	// ARUint8 *myRGBABuffer = (ARUint8 *)malloc(videoWidth * videoHeight * 4);
	gPixFormat = AR_PIXEL_FORMAT_NV21;
	gVideoFrameSize = (sizeof(ARUint8)*(w*h + 2*w/2*h/2));
	gVideoFrame = (ARUint8*) (malloc(gVideoFrameSize));
	if (!gVideoFrame) {
		gVideoFrameSize = 0;
		LOGE("Error allocating frame buffer");
		return false;
	}
	pthread_mutex_init(&gVideoFrameLock, NULL);
	videoWidth = w;
	videoHeight = h;
	gCameraIndex = cameraIndex;
	gCameraIsFrontFacing = cameraIsFrontFacing;
	LOGI("Video camera %d (%s), %dx%d format %s, %d-byte buffer.", gCameraIndex, (gCameraIsFrontFacing ? "front" : "rear"), w, h, arUtilGetPixelFormatName(gPixFormat), gVideoFrameSize);

	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_WIDTH, videoWidth);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_HEIGHT, videoHeight);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_PIXELFORMAT, (int)gPixFormat);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_CAMERA_INDEX, gCameraIndex);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_CAMERA_FACE, gCameraIsFrontFacing);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_INTERNET_STATE, gInternetState);

	if (ar2VideoGetCParamAsync(gVid, nativeVideoGetCparamCallback, NULL) < 0) {
		LOGE("Error getting cparam.\n");
		nativeVideoGetCparamCallback(NULL, NULL);
	}

	return (true);
}

static void nativeVideoGetCparamCallback(const ARParam *cparam_p, void *userdata)
{
	// Load the camera parameters, resize for the window and init.
	ARParam cparam;
	if (cparam_p) cparam = *cparam_p;
	else {
        arParamClearWithFOVy(&cparam, videoWidth, videoHeight, M_PI_4); // M_PI_4 radians = 45 degrees.
        LOGE("Using default camera parameters for %dx%d image size, 45 degrees vertical field-of-view.", videoWidth, videoHeight);
	}
	if (cparam.xsize != videoWidth || cparam.ysize != videoHeight) {
#ifdef DEBUG
		LOGI("*** Camera Parameter resized from %d, %d. ***\n", cparam.xsize, cparam.ysize);
#endif
		arParamChangeSize(&cparam, videoWidth, videoHeight, &cparam);
	}
#ifdef DEBUG
	LOGI("*** Camera Parameter ***\n");
	arParamDisp(&cparam);
#endif
	if ((gCparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
		LOGE("Error: arParamLTCreate.\n");
		return;
	}
	videoInited = true;

	//
	// AR init.
	//
    
	// Create the OpenGL projection from the calibrated camera parameters.
	arglCameraFrustumRHf(&gCparamLT->param, NEAR_PLANE, FAR_PLANE, cameraLens);
	cameraPoseValid = FALSE;

	if (!initNFT(gCparamLT, gPixFormat)) {
		LOGE("Error initialising NFT.\n");
		arParamLTFree(&gCparamLT);
		return;
	}

	// Marker data has already been loaded, so now load NFT data on a second thread.
	nftDataLoadingThreadHandle = threadInit(0, NULL, loadNFTDataAsync);
	if (!nftDataLoadingThreadHandle) {
		LOGE("Error starting NFT loading thread.\n");
		arParamLTFree(&gCparamLT);
		return;
	}
	threadStartSignal(nftDataLoadingThreadHandle);

}

// References globals:
// Modifies globals: kpmHandle, ar2Handle.
static int initNFT(ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat)
{
#ifdef DEBUG
    LOGE("Initialising NFT.\n");
#endif
    //
    // NFT init.
    //
    
    // KPM init.
    kpmHandle = kpmCreateHandle(cparamLT);
    if (!kpmHandle) {
        LOGE("Error: kpmCreatHandle.\n");
        return (false);
    }
    //kpmSetProcMode( kpmHandle, KpmProcHalfSize );
    
    // AR2 init.
    if( (ar2Handle = ar2CreateHandle(cparamLT, pixFormat, AR2_TRACKING_DEFAULT_THREAD_NUM)) == NULL ) {
        LOGE("Error: ar2CreateHandle.\n");
        kpmDeleteHandle(&kpmHandle);
        return (false);
    }
    if (threadGetCPU() <= 1) {
#ifdef DEBUG
        LOGE("Using NFT tracking settings for a single CPU.\n");
#endif
        ar2SetTrackingThresh( ar2Handle, 5.0 );
        ar2SetSimThresh( ar2Handle, 0.50 );
        ar2SetSearchFeatureNum(ar2Handle, 16);
        ar2SetSearchSize(ar2Handle, 6);
        ar2SetTemplateSize1(ar2Handle, 6);
        ar2SetTemplateSize2(ar2Handle, 6);
    } else {
#ifdef DEBUG
        LOGE("Using NFT tracking settings for more than one CPU.\n");
#endif
        ar2SetTrackingThresh( ar2Handle, 5.0 );
        ar2SetSimThresh( ar2Handle, 0.50 );
        ar2SetSearchFeatureNum(ar2Handle, 16);
        ar2SetSearchSize(ar2Handle, 12);
        ar2SetTemplateSize1(ar2Handle, 6);
        ar2SetTemplateSize2(ar2Handle, 6);
    }
    // NFT dataset loading will happen later.
#ifdef DEBUG
    LOGE("NFT initialised OK.\n");
#endif
    return (true);
}

// References globals: markersNFTCount
// Modifies globals: trackingThreadHandle, surfaceSet[], surfaceSetCount, markersNFT[], markersNFTCount
static void *loadNFTDataAsync(THREAD_HANDLE_T *threadHandle)
{
    int i, j;
	KpmRefDataSet *refDataSet;
    
    while (threadStartWait(threadHandle) == 0) {
#ifdef DEBUG
        LOGE("Loading NFT data.\n");
#endif
    
		// If data was already loaded, stop KPM tracking thread and unload previously loaded data.
		if (trackingThreadHandle) {
			LOGE("NFT2 tracking thread is running. Stopping it first.\n");
			trackingInitQuit(&trackingThreadHandle);
			detectedPage = -2;
		}
		j = 0;
		for (i = 0; i < surfaceSetCount; i++) {
			if (j == 0) LOGE("Unloading NFT tracking surfaces.");
			ar2FreeSurfaceSet(&surfaceSet[i]); // Also sets surfaceSet[i] to NULL.
			j++;
		}
		if (j > 0) LOGE("Unloaded %d NFT tracking surfaces.\n", j);
		surfaceSetCount = 0;

		refDataSet = NULL;

		for (i = 0; i < markersNFTCount; i++) {
			// Load KPM data.
			KpmRefDataSet  *refDataSet2;
			LOGI("Reading %s.fset3\n", markersNFT[i].datasetPathname);
			if (kpmLoadRefDataSet(markersNFT[i].datasetPathname, "fset3", &refDataSet2) < 0 ) {
				LOGE("Error reading KPM data from %s.fset3\n", markersNFT[i].datasetPathname);
				markersNFT[i].pageNo = -1;
				continue;
			}
			markersNFT[i].pageNo = surfaceSetCount;
			LOGI("  Assigned page no. %d.\n", surfaceSetCount);
			if (kpmChangePageNoOfRefDataSet(refDataSet2, KpmChangePageNoAllPages, surfaceSetCount) < 0) {
				LOGE("Error: kpmChangePageNoOfRefDataSet\n");
				exit(-1);
			}
			if (kpmMergeRefDataSet(&refDataSet, &refDataSet2) < 0) {
				LOGE("Error: kpmMargeRefDataSet\n");
				exit(-1);
			}
			LOGI("  Done.\n");

			// Load AR2 data.
			LOGI("Reading %s.fset\n", markersNFT[i].datasetPathname);

			if ((surfaceSet[surfaceSetCount] = ar2ReadSurfaceSet(markersNFT[i].datasetPathname, "fset", NULL)) == NULL ) {
				LOGE("Error reading data from %s.fset\n", markersNFT[i].datasetPathname);
			}
			LOGI("  Done.\n");

			surfaceSetCount++;
			if (surfaceSetCount == PAGES_MAX) break;
		}
		if (kpmSetRefDataSet(kpmHandle, refDataSet) < 0) {
			LOGE("Error: kpmSetRefDataSet");
			exit(-1);
		}
		kpmDeleteRefDataSet(&refDataSet);

		// Start the KPM tracking thread.
		trackingThreadHandle = trackingInitInit(kpmHandle);
		if (!trackingThreadHandle) exit(-1);

#ifdef DEBUG
        LOGI("Loading of NFT data complete.");
#endif

        threadEndSignal(threadHandle); // Signal that we're done.
    }
    return (NULL); // Exit this thread.
}

JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeVideoFrame(JNIEnv* env, jobject obj, jbyteArray pinArray))
{
    int i, j, k;
    jbyte* inArray;
        
    if (!videoInited) {
#ifdef DEBUG
        LOGI("nativeVideoFrame !VIDEO\n");
#endif        
        return; // No point in trying to track until video is inited.
    }
    if (!nftDataLoaded) {
#ifdef DEBUG
        LOGI("nativeVideoFrame !NFTDATA\n");
#endif        
        return;
    }
    if (!gARViewInited) {
        return; // Also, we won't track until the ARView has been inited.
#ifdef DEBUG
        LOGI("nativeVideoFrame !ARVIEW\n");
#endif        
    }
#ifdef DEBUG
    LOGI("nativeVideoFrame\n");
#endif        
    
    pthread_mutex_lock(&gVideoFrameLock);

    // Copy the incoming  YUV420 image in pinArray.
    env->GetByteArrayRegion(pinArray, 0, gVideoFrameSize, (jbyte *)gVideoFrame);
    
	// As of ARToolKit v5.0, NV21 format video frames are handled natively,
	// and no longer require colour conversion to RGBA.
	// If you still require RGBA format information from the video,
    // here is where you'd do the conversion:
    // color_convert_common(gVideoFrame, gVideoFrame + videoWidth*videoHeight, videoWidth, videoHeight, myRGBABuffer);

    videoFrameNeedsPixelBufferDataUpload = true; // Note that buffer needs uploading. (Upload must be done on OpenGL context's thread.)
    pthread_mutex_unlock(&gVideoFrameLock);

    // Run marker detection on frame
    if (trackingThreadHandle) {
        // Perform NFT tracking.
        float            err;
        int              ret;
        int              pageNo;
        
        if( detectedPage == -2 ) {
            trackingInitStart( trackingThreadHandle, gVideoFrame );
            detectedPage = -1;
        }
        if( detectedPage == -1 ) {
            ret = trackingInitGetResult( trackingThreadHandle, trackingTrans, &pageNo);
            if( ret == 1 ) {
                if (pageNo >= 0 && pageNo < surfaceSetCount) {
//#ifdef DEBUG
                    LOGE("Detected page %d.\n", pageNo);
//#endif
                    detectedPage = pageNo;
                    ar2SetInitTrans(surfaceSet[detectedPage], trackingTrans);
                } else {
                    LOGE("Detected bad page %d.\n", pageNo);
                    detectedPage = -2;
                }
            } else if( ret < 0 ) {
#ifdef DEBUG
                LOGE("No page detected.\n");
#endif
                detectedPage = -2;
            }
        }
        if( detectedPage >= 0 && detectedPage < surfaceSetCount) {
            if( ar2Tracking(ar2Handle, surfaceSet[detectedPage], gVideoFrame, trackingTrans, &err) < 0 ) {
//#ifdef DEBUG
                LOGE("Tracking lost.\n");
//#endif
                detectedPage = -2;
            } else {
#ifdef DEBUG
                LOGE("Tracked page %d (max %d).\n", detectedPage, surfaceSetCount - 1);
#endif
            }
        }
    } else {
        LOGE("Error: trackingThreadHandle\n");
        detectedPage = -2;
    }
    
    // Update markers.
    for (i = 0; i < markersNFTCount; i++) {
        markersNFT[i].validPrev = markersNFT[i].valid;
        if (markersNFT[i].pageNo >= 0 && markersNFT[i].pageNo == detectedPage) {
            markersNFT[i].valid = TRUE;
            for (j = 0; j < 3; j++) for (k = 0; k < 4; k++) markersNFT[i].trans[j][k] = trackingTrans[j][k];
        }
        else markersNFT[i].valid = FALSE;
        if (markersNFT[i].valid) {
            
            // Filter the pose estimate.
            if (markersNFT[i].ftmi) {
                if (arFilterTransMat(markersNFT[i].ftmi, markersNFT[i].trans, !markersNFT[i].validPrev) < 0) {
                    LOGE("arFilterTransMat error with marker %d.\n", i);
                }
            }
            
            if (!markersNFT[i].validPrev) {
                // Marker has become visible, tell any dependent objects.
                //ARMarkerAppearedNotification
                
                if (env && mMovieJClass && mMoviePlayJMethodID && mMovieJObjectWeak) {
                    env->CallStaticVoidMethod(mMovieJClass, mMoviePlayJMethodID, mMovieJObjectWeak); // play().
                }
            }
    
            // We have a new pose, so set that.
            arglCameraViewRHf(markersNFT[i].trans, markersNFT[i].pose.T, 1.0f /*VIEW_SCALEFACTOR*/);
            // Tell any dependent objects about the update.
            //ARMarkerUpdatedPoseNotification
            
        } else {
            
            if (markersNFT[i].validPrev) {
                // Marker has ceased to be visible, tell any dependent objects.
                //ARMarkerDisappearedNotification
                
                if (env && mMovieJClass && mMoviePauseJMethodID && mMovieJObjectWeak) {
                    env->CallStaticVoidMethod(mMovieJClass, mMoviePauseJMethodID, mMovieJObjectWeak); // pause()
                }
            }
        }                    
    }
}

//
// OpenGL functions.
//

//
// This is called whenever the OpenGL context has just been created or re-created.
// Note that GLSurfaceView is a bit asymmetrical here; we don't get a call when the
// OpenGL context is about to be deleted, it's just whipped out from under us. So it's
// possible that when we enter this function, we're actually resuming after such an
// event. What about resources we allocated previously which we didn't get time to
// de-allocate? Well, we don't have to worry about the OpenGL resources themselves, they
// were deleted along with the context. But, we should clean up any data structures we
// allocated with malloc etc. ARGL's settings falls into this category.
//
JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSurfaceCreated(JNIEnv* env, jobject object))
{
#ifdef DEBUG
    LOGI("nativeSurfaceCreated\n");
#endif        
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glStateCacheFlush(); // Make sure we don't hold outdated OpenGL state.

	if (gArglSettings) {
		arglCleanup(gArglSettings); // Clean up any left-over ARGL data.
		gArglSettings = NULL;
	}
	
    gARViewInited = false;
}

//
// This is called when something about the surface changes. e.g. size.
//
// Modifies globals: backingWidth, backingHeight, gARViewLayoutRequired.
JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSurfaceChanged(JNIEnv* env, jobject object, jint w, jint h))
{
    backingWidth = w;
    backingHeight = h;
#ifdef DEBUG
    LOGI("nativeSurfaceChanged backingWidth=%d, backingHeight=%d\n", w, h);
#endif        
    
	// Call through to anyone else who needs to know about window sizing here.

    // In order to do something meaningful with the surface backing size in an AR sense,
    // we also need the content size, which we aren't guaranteed to have yet, so defer
    // the viewPort calculations.
    gARViewLayoutRequired = true;
}

// 0 = portrait, 1 = landscape (device rotated 90 degrees ccw), 2 = portrait upside down, 3 = landscape reverse (device rotated 90 degrees cw).
JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDisplayParametersChanged(JNIEnv* env, jobject object, jint orientation, jint width, jint height, jint dpi))
{
#ifdef DEBUG
    LOGI("nativeDisplayParametersChanged orientation=%d, size=%dx%d@%dpi\n", orientation, width, height, dpi);
#endif
	gDisplayOrientation = orientation;
	gDisplayWidth = width;
	gDisplayHeight = height;
	gDisplayDPI = dpi;

    gARViewLayoutRequired = true;
}

JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSetInternetState(JNIEnv* env, jobject obj, jint state))
{
	gInternetState = state;
	if (gVid) {
		ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_INTERNET_STATE, state);
	}
}

// Lays out the AR view. Requires both video and OpenGL to be inited, and must be called on OpenGL thread.
// References globals: gContentMode, backingWidth, backingHeight, videoWidth, videoHeight, .
// Modifies globals: gContentFlipV, gContentFlipH, gContentRotate90, viewPort, gARViewLayoutRequired.
static bool layoutARView(void)
{
	if (gDisplayOrientation == 0) {
		gContentRotate90 = true;
		gContentFlipV = false;
		gContentFlipH = gCameraIsFrontFacing;
	} else if (gDisplayOrientation == 1) {
		gContentRotate90 = false;
		gContentFlipV = false;
		gContentFlipH = gCameraIsFrontFacing;
	} else if (gDisplayOrientation == 2) {
		gContentRotate90 = true;
		gContentFlipV = true;
		gContentFlipH = (!gCameraIsFrontFacing);
	} else if (gDisplayOrientation == 3) {
		gContentRotate90 = false;
		gContentFlipV = true;
		gContentFlipH = (!gCameraIsFrontFacing);
	}
    arglSetRotate90(gArglSettings, gContentRotate90);
    arglSetFlipV(gArglSettings, gContentFlipV);
    arglSetFlipH(gArglSettings, gContentFlipH);

    // Calculate viewPort.
    int left, bottom, w, h;
    int contentWidth = videoWidth;
    int contentHeight = videoHeight;

	if (gContentMode == ARViewContentModeScaleToFill) {
        w = backingWidth;
        h = backingHeight;
    } else {
        int contentWidthFinalOrientation = (gContentRotate90 ? contentHeight : contentWidth);
        int contentHeightFinalOrientation = (gContentRotate90 ? contentWidth : contentHeight);
        if (gContentMode == ARViewContentModeScaleAspectFit || gContentMode == ARViewContentModeScaleAspectFill) {
            float scaleRatioWidth, scaleRatioHeight, scaleRatio;
            scaleRatioWidth = (float)backingWidth / (float)contentWidthFinalOrientation;
            scaleRatioHeight = (float)backingHeight / (float)contentHeightFinalOrientation;
            if (gContentMode == ARViewContentModeScaleAspectFill) scaleRatio = MAX(scaleRatioHeight, scaleRatioWidth);
            else scaleRatio = MIN(scaleRatioHeight, scaleRatioWidth);
            w = (int)((float)contentWidthFinalOrientation * scaleRatio);
            h = (int)((float)contentHeightFinalOrientation * scaleRatio);
        } else {
            w = contentWidthFinalOrientation;
            h = contentHeightFinalOrientation;
        }
    }
    
    if (gContentMode == ARViewContentModeTopLeft
        || gContentMode == ARViewContentModeLeft
        || gContentMode == ARViewContentModeBottomLeft) left = 0;
    else if (gContentMode == ARViewContentModeTopRight
             || gContentMode == ARViewContentModeRight
             || gContentMode == ARViewContentModeBottomRight) left = backingWidth - w;
    else left = (backingWidth - w) / 2;
        
    if (gContentMode == ARViewContentModeBottomLeft
        || gContentMode == ARViewContentModeBottom
        || gContentMode == ARViewContentModeBottomRight) bottom = 0;
    else if (gContentMode == ARViewContentModeTopLeft
             || gContentMode == ARViewContentModeTop
             || gContentMode == ARViewContentModeTopRight) bottom = backingHeight - h;
    else bottom = (backingHeight - h) / 2;

    glViewport(left, bottom, w, h);
    
    viewPort[viewPortIndexLeft] = left;
    viewPort[viewPortIndexBottom] = bottom;
    viewPort[viewPortIndexWidth] = w;
    viewPort[viewPortIndexHeight] = h;
    
#ifdef DEBUG
    LOGE("Viewport={%d, %d, %d, %d}\n", left, bottom, w, h);
#endif
    // Call through to anyone else who needs to know about changes in the ARView layout here.
    // --->

    gARViewLayoutRequired = false;
    
    return (true);
}


// All tasks which require both video and OpenGL to be inited should be performed here.
// References globals: gCparamLT, gPixFormat
// Modifies globals: gArglSettings
static bool initARView(void)
{
#ifdef DEBUG
    LOGI("Initialising ARView\n");
#endif        
    if (gARViewInited) return (false);
    
#ifdef DEBUG
    LOGI("Setting up argl.\n");
#endif        
    if ((gArglSettings = arglSetupForCurrentContext(&gCparamLT->param, gPixFormat)) == NULL) {
        LOGE("Unable to setup argl.\n");
        return (false);
    }
#ifdef DEBUG
    LOGI("argl setup OK.\n");
#endif        

    gARViewInited = true;
#ifdef DEBUG
    LOGI("ARView initialised.\n");
#endif

    return (true);
}

JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDrawFrame(JNIEnv* env, jobject obj, jint movieWidth, jint movieHeight, jint movieTextureID, jfloatArray movieTextureMtx))
{
	float width, height;
	
	// Get the array contents.
	//jsize movieTextureMtxLen = env->GetArrayLength(movieTextureMtx);
	float movieTextureMtxUnpacked[16];
    env->GetFloatArrayRegion(movieTextureMtx, 0, /*movieTextureMtxLen*/ 16, movieTextureMtxUnpacked);
        
    if (!videoInited) {
#ifdef DEBUG
        LOGI("nativeDrawFrame !VIDEO\n");
#endif        
        return; // No point in trying to draw until video is inited.
    }
    if (!nftDataLoaded && nftDataLoadingThreadHandle) {
        // Check if NFT data loading has completed.
        if (threadGetStatus(nftDataLoadingThreadHandle) > 0) {
            nftDataLoaded = true;
            threadWaitQuit(nftDataLoadingThreadHandle);
            threadFree(&nftDataLoadingThreadHandle); // Clean up.
        } else {
#ifdef DEBUG
            LOGI("nativeDrawFrame !NFTDATA\n");
#endif        
            return; // No point in trying to draw until NFT data is loaded.
        }
    }
#ifdef DEBUG
    LOGI("nativeDrawFrame\n");
#endif        
    if (!gARViewInited) {
        if (!initARView()) return;
    }
    if (gARViewLayoutRequired) layoutARView();
    
    // Upload new video frame if required.
    if (videoFrameNeedsPixelBufferDataUpload) {
        pthread_mutex_lock(&gVideoFrameLock);
        arglPixelBufferDataUploadBiPlanar(gArglSettings, gVideoFrame, gVideoFrame + videoWidth*videoHeight);
        videoFrameNeedsPixelBufferDataUpload = false;
        pthread_mutex_unlock(&gVideoFrameLock);
    }
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers for new frame.
    
    // Display the current frame
    arglDispImage(gArglSettings);
    
    // Set up 3D mode.
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(cameraLens);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    glStateCacheEnableDepthTest();

    // Set any initial per-frame GL state you require here.
    // --->
    
    // Lighting and geometry that moves with the camera should be added here.
    // (I.e. should be specified before camera pose transform.)
    // --->
        
    // Draw an object on all valid markers.
    for (int i = 0; i < markersNFTCount; i++) {
        if (markersNFT[i].valid) {
            glLoadMatrixf(markersNFT[i].pose.T);
            
            //
            // Draw a rectangular surface textured with the movie texture.
            //
            float w = 80.0f;
            float h = w * (float)movieHeight/(float)movieWidth;
            GLfloat vertices[4][2] = { {0.0f, 0.0f}, {w, 0.0f}, {w, h}, {0.0f, h} };
            GLfloat normals[4][3] = { {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} };
            GLfloat texcoords[4][2] = { {0.0f, 0.0f},  {1.0f, 0.0f},  {1.0f, 1.0f},  {0.0f, 1.0f} };

            glStateCacheActiveTexture(GL_TEXTURE0);

            glMatrixMode(GL_TEXTURE);
            glPushMatrix();
            glLoadMatrixf(movieTextureMtxUnpacked);
            glMatrixMode(GL_MODELVIEW);
            
            glVertexPointer(2, GL_FLOAT, 0, vertices);
            glNormalPointer(GL_FLOAT, 0, normals);
            glStateCacheClientActiveTexture(GL_TEXTURE0);
            glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
            glStateCacheEnableClientStateVertexArray();
            glStateCacheEnableClientStateNormalArray();
            glStateCacheEnableClientStateTexCoordArray();
            glStateCacheBindTexture2D(0);
            glStateCacheDisableTex2D();
            glStateCacheDisableLighting();

            glEnable(GL_TEXTURE_EXTERNAL_OES);
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, movieTextureID);

            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
            glDisable(GL_TEXTURE_EXTERNAL_OES);

            glMatrixMode(GL_TEXTURE);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            //
            // End.
            //
        }
    }
    
    if (cameraPoseValid) {
        
        glMultMatrixf(cameraPose);
        
        // All lighting and geometry to be drawn in world coordinates goes here.
        // --->
    }
        
    // If you added external OpenGL code above, and that code doesn't use the glStateCache routines,
    // then uncomment the line below.
    //glStateCacheFlush();
    
    // Set up 2D mode.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	width = (float)viewPort[viewPortIndexWidth];
	height = (float)viewPort[viewPortIndexHeight];
	glOrthof(0.0f, width, 0.0f, height, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glStateCacheDisableDepthTest();

    // Add your own 2D overlays here.
    // --->
    
    // If you added external OpenGL code above, and that code doesn't use the glStateCache routines,
    // then uncomment the line below.
    //glStateCacheFlush();

#ifdef DEBUG
    // Example of 2D drawing. It just draws a white border line. Change the 0 to 1 to enable.
    const GLfloat square_vertices [4][2] = { {0.5f, 0.5f}, {0.5f, height - 0.5f}, {width - 0.5f, height - 0.5f}, {width - 0.5f, 0.5f} };
    glStateCacheDisableLighting();
    glStateCacheDisableTex2D();
    glVertexPointer(2, GL_FLOAT, 0, square_vertices);
    glStateCacheEnableClientStateVertexArray();
    glColor4ub(255, 255, 255, 255);
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    CHECK_GL_ERROR();
#endif
}

// Make the connections to the Java MovieController instance.
JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeMovieInit(JNIEnv* env, jobject obj, jobject movieControllerThis, jobject movieControllerWeakThis))
{
#ifdef DEBUG
    LOGI("nativeMovieInit()\n");
#endif
    if (!movieControllerThis || !movieControllerWeakThis) {
        LOGE("Error: natitveMovieInit called with null reference.\n");
        return;
    }
    
    //jclass classOfCaller = (jclass)obj; // Static native method gets class as second arg.
    jclass movieJClass = env->GetObjectClass(movieControllerThis);
    if (!movieJClass) {
        LOGE("Error: nativeMovieInit() couldn't get class.");
        return;
    }
    mMovieJClass = (jclass)env->NewGlobalRef(movieJClass);
    env->DeleteLocalRef(movieJClass);
    if (!mMovieJClass) {
        LOGE("Error: nativeMovieInit() couldn't create global ref.");
        return;
    }
    mMoviePlayJMethodID = env->GetStaticMethodID(mMovieJClass, "playFromNative", "(Ljava/lang/Object;)V");
    mMoviePauseJMethodID = env->GetStaticMethodID(mMovieJClass, "pauseFromNative", "(Ljava/lang/Object;)V");
    if (!mMoviePlayJMethodID || !mMoviePauseJMethodID) {
        LOGE("Error: nativeMovieInit() couldn't get method IDs.");
        return;
    }
    mMovieJObjectWeak = env->NewGlobalRef(movieControllerWeakThis);
    if (!mMovieJObjectWeak) {
        LOGE("Error: nativeMovieInit() couldn't create global ref.");
        return;
    }
}

// Break the connections to the Java MovieController instance.
JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeMovieFinal(JNIEnv* env, jobject obj))
{
#ifdef DEBUG
    LOGI("nativeMovieFinal()\n");
#endif
    if (mMovieJClass != NULL) {
        env->DeleteGlobalRef(mMovieJClass);
        mMovieJClass = NULL;
    }
    mMoviePlayJMethodID = NULL;
    mMoviePauseJMethodID = NULL;
    if (mMovieJObjectWeak != NULL) {
        env->DeleteGlobalRef(mMovieJObjectWeak);
        mMovieJObjectWeak = NULL;
    }
}
