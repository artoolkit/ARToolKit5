/*
 *  ARNativeOSG.cpp
 *  ARToolKit for Android
 *
 *  An example with all ARToolKit setup performed in native code,
 *  and with models rendered using OpenSceneGraph via the VirtualEnvironment pseudo-class.
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

#include <AR/ar.h>
#include <AR/arMulti.h>
#include <AR/video.h>
#include <AR/gsub_es.h>
#include <AR/arFilterTransMat.h>
#include <AR/arosg.h>

#include "ARMarkerSquare.h"
#include "VirtualEnvironment.h"
#include "osgPlugins.h"

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

#ifndef MAX
#  define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#endif
#ifndef MIN
#  define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif

// Logging macros
#define  LOG_TAG    "ARNativeOSGNative"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

// ============================================================================
//	Android NDK function signatures
// ============================================================================

// Utility preprocessor directive so only one change needed if Java class name changes
#define JNIFUNCTION_NATIVE(sig) Java_org_artoolkit_ar_samples_ARNativeOSG_ARNativeOSGActivity_##sig

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
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDrawFrame(JNIEnv* env, jobject obj));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSetInternetState(JNIEnv* env, jobject obj, jint state));
};

static void nativeVideoGetCparamCallback(const ARParam *cparam, void *userdata);

// ============================================================================
//	Global variables
// ============================================================================

// Preferences.
static const char *markerConfigDataFilename = "Data/markers.dat";
static const char *objectDataFilename = "Data/objects.dat";

// Image acquisition.
static AR2VideoParamT *gVid = NULL;
static bool videoInited = false;                                    ///< true when ready to receive video frames.
static int videoWidth = 0;                                          ///< Width of the video frame in pixels.
static int videoHeight = 0;                                         ///< Height of the video frame in pixels.
static AR_PIXEL_FORMAT gPixFormat;                                  ///< Pixel format from ARToolKit enumeration.
static ARUint8* gVideoFrame = NULL;                                 ///< Buffer containing current video frame.
static size_t gVideoFrameSize = 0;                                  ///< Size of buffer containing current video frame.
static AR2VideoBufferT *gVideoBuffer = NULL;                        ///< Buffer containing current video frame.
static bool videoFrameNeedsPixelBufferDataUpload = false;
static int gCameraIndex = 0;
static bool gCameraIsFrontFacing = false;

// Markers.
static ARMarkerSquare *markersSquare = NULL;
static int markersSquareCount = 0;

// Tracking.
static ARHandle* arHandle;											///< Structure containing general ARToolKit tracking information 
static ARPattHandle* arPattHandle;									///< Structure containing information about trained patterns
static AR3DHandle* ar3DHandle;										///< Structure used to compute 3D poses from tracking data
static int arPattDetectionMode;

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
    arPattHandle = arPattCreateHandle();
	if (arPattHandle == NULL) {
		LOGE("Error creating pattern handle");
		return false;
	}
    newMarkers(markerConfigDataFilename, arPattHandle, &markersSquare, &markersSquareCount, &arPattDetectionMode);
    if (!markersSquareCount) {
        LOGE("Error loading markers from config. file '%s'.", markerConfigDataFilename);
        arPattDeleteHandle(arPattHandle);
        arPattHandle = NULL;
        return false;
    }
#ifdef DEBUG
    LOGE("Marker count = %d\n", markersSquareCount);
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
    
    // Since most AR init can't be completed until the video frame size is known,
    // that will be deferred.
    
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
    
	// Can't call arglCleanup() or VirtualEnvironmentFinal() here, because nativeStop is not called on rendering thread.

    // Clean up ARToolKit data.
    if (ar3DHandle) ar3DDeleteHandle(&ar3DHandle);
    if (arHandle) {
        arPattDetach(arHandle);
        arDeleteHandle(arHandle);
        arHandle = NULL;
    }
    arParamLTFree(&gCparamLT);
    
    // OpenGL cleanup -- not done here.
    
    // Video cleanup.
    if (gVideoFrame) {
        free(gVideoFrame);
        gVideoFrame = NULL;
        gVideoFrameSize = 0;
    }
    if (gVideoBuffer) {
    	free(gVideoBuffer->bufPlanes);
    	free(gVideoBuffer);
    	gVideoBuffer = NULL;
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
    if (markersSquare) deleteMarkers(&markersSquare, &markersSquareCount, arPattHandle);
    if (arPattHandle) {
        arPattDeleteHandle(arPattHandle);
        arPattHandle = NULL;
    }
   
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
	gVideoFrame = (ARUint8 *)malloc(gVideoFrameSize);
	if (!gVideoFrame) {
		gVideoFrameSize = 0;
		LOGE("Error allocating frame buffer");
		return false;
	}
	videoWidth = w;
	videoHeight = h;
	gVideoBuffer = (AR2VideoBufferT *)calloc(1, sizeof(AR2VideoBufferT));
	gVideoBuffer->bufPlanes = (ARUint8 **)calloc(2, sizeof(ARUint8 *));
	gVideoBuffer->bufPlaneCount = 2;
	gVideoBuffer->bufPlanes[0] = gVideoFrame;
	gVideoBuffer->bufPlanes[1] = gVideoFrame + videoWidth*videoHeight;
	gVideoBuffer->buff = gVideoBuffer->buffLuma = gVideoFrame;
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

    // Init AR.
    arHandle = arCreateHandle(gCparamLT);
    if (arHandle == NULL) {
        LOGE("Error creating AR handle");
        return;
    }
    arPattAttach(arHandle, arPattHandle);

    if (arSetPixelFormat(arHandle, gPixFormat) < 0) {
        LOGE("Error setting pixel format");
        return;
    }

    ar3DHandle = ar3DCreateHandle(&gCparamLT->param);
    if (ar3DHandle == NULL) {
        LOGE("Error creating 3D handle");
        return;
    }
            
    // Other ARToolKit setup. 
    arSetMarkerExtractionMode(arHandle, AR_USE_TRACKING_HISTORY_V2);
    //arSetMarkerExtractionMode(arHandle, AR_NOUSE_TRACKING_HISTORY);
    //arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_MANUAL); // Uncomment to use  manual thresholding.

    // Set the pattern detection mode (template (pictorial) vs. matrix (barcode) based on
    // the marker types as defined in the marker config. file.
    arSetPatternDetectionMode(arHandle, arPattDetectionMode); // Default = AR_TEMPLATE_MATCHING_COLOR

    // Other application-wide marker options. Once set, these apply to all markers in use in the application.
    // If you are using standard ARToolKit picture (template) markers, leave commented to use the defaults.
    // If you are usign a different marker design (see http://www.artoolworks.com/support/app/marker.php )
    // then uncomment and edit as instructed by the marker design application.
    //arSetLabelingMode(arHandle, AR_LABELING_BLACK_REGION); // Default = AR_LABELING_BLACK_REGION
    //arSetBorderSize(arHandle, 0.25f); // Default = 0.25f
    //arSetMatrixCodeType(arHandle, AR_MATRIX_CODE_3x3); // Default = AR_MATRIX_CODE_3x3
}

JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeVideoFrame(JNIEnv* env, jobject obj, jbyteArray pinArray))
{
    int i, j, k;
    jbyte* inArray;
    ARdouble err;
        
    if (!videoInited) {
#ifdef DEBUG
        LOGI("nativeVideoFrame !VIDEO\n");
#endif        
        return; // No point in trying to track until video is inited.
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
    
    // Copy the incoming  YUV420 image in pinArray.
    env->GetByteArrayRegion(pinArray, 0, gVideoFrameSize, (jbyte *)gVideoFrame);
    
	// As of ARToolKit v5.0, NV21 format video frames are handled natively,
	// and no longer require colour conversion to RGBA.
	// If you still require RGBA format information from the video,
    // here is where you'd do the conversion:
    // color_convert_common(gVideoFrame, gVideoFrame + videoWidth*videoHeight, videoWidth, videoHeight, myRGBABuffer);

    videoFrameNeedsPixelBufferDataUpload = true; // Note that buffer needs uploading. (Upload must be done on OpenGL context's thread.)
    
	// Run marker detection on frame
	arDetectMarker(arHandle, gVideoBuffer);
	
	// Get detected markers
	ARMarkerInfo* markerInfo = arGetMarker(arHandle);
	int markerNum = arGetMarkerNum(arHandle);
    
    // Update markers.
    for (i = 0; i < markersSquareCount; i++) {
        markersSquare[i].validPrev = markersSquare[i].valid;
        
        
        // Check through the marker_info array for highest confidence
        // visible marker matching our preferred pattern.
        k = -1;
        if (markersSquare[i].patt_type == AR_PATTERN_TYPE_TEMPLATE) {
            for (j = 0; j < markerNum; j++) {
                if (markersSquare[i].patt_id == markerInfo[j].idPatt) {
                    if (k == -1) {
                        if (markerInfo[j].cfPatt >= markersSquare[i].matchingThreshold) k = j; // First marker detected.
                    } else if (markerInfo[j].cfPatt > markerInfo[k].cfPatt) k = j; // Higher confidence marker detected.
                }
            }
            if (k != -1) {
                markerInfo[k].id = markerInfo[k].idPatt;
                markerInfo[k].cf = markerInfo[k].cfPatt;
                markerInfo[k].dir = markerInfo[k].dirPatt;
            }
        } else {
            for (j = 0; j < markerNum; j++) {
                if (markersSquare[i].patt_id == markerInfo[j].idMatrix) {
                    if (k == -1) {
                        if (markerInfo[j].cfMatrix >= markersSquare[i].matchingThreshold) k = j; // First marker detected.
                    } else if (markerInfo[j].cfMatrix > markerInfo[k].cfMatrix) k = j; // Higher confidence marker detected.
                }
            }
            if (k != -1) {
                markerInfo[k].id = markerInfo[k].idMatrix;
                markerInfo[k].cf = markerInfo[k].cfMatrix;
                markerInfo[k].dir = markerInfo[k].dirMatrix;
            }
        }

        if (k != -1) {
            markersSquare[i].valid = TRUE;
#ifdef DEBUG
            LOGI("Marker %d matched pattern %d.\n", k, markerInfo[k].id);
#endif
            // Get the transformation between the marker and the real camera into trans.
            if (markersSquare[i].validPrev) {
                err = arGetTransMatSquareCont(ar3DHandle, &(markerInfo[k]), markersSquare[i].trans, markersSquare[i].marker_width, markersSquare[i].trans);
            } else {
                err = arGetTransMatSquare(ar3DHandle, &(markerInfo[k]), markersSquare[i].marker_width, markersSquare[i].trans);
            }
        } else {
            markersSquare[i].valid = FALSE;
        }
       
        if (markersSquare[i].valid) {
            
            // Filter the pose estimate.
            if (markersSquare[i].ftmi) {
                if (arFilterTransMat(markersSquare[i].ftmi, markersSquare[i].trans, !markersSquare[i].validPrev) < 0) {
                    LOGE("arFilterTransMat error with marker %d.\n", i);
                }
            }
            
            if (!markersSquare[i].validPrev) {
                // Marker has become visible, tell any dependent objects.
                VirtualEnvironmentHandleARMarkerAppeared(i);
            }
    
            // We have a new pose, so set that.
            arglCameraViewRHf(markersSquare[i].trans, markersSquare[i].pose.T, 1.0f /*VIEW_SCALEFACTOR*/);
            // Tell any dependent objects about the update.
            VirtualEnvironmentHandleARMarkerWasUpdated(i, markersSquare[i].pose);
            
        } else {
            
            if (markersSquare[i].validPrev) {
                // Marker has ceased to be visible, tell any dependent objects.
                VirtualEnvironmentHandleARMarkerDisappeared(i);
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
	VirtualEnvironmentFinal(); // Clean up any left-over OSG data.
	
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
    VirtualEnvironmentHandleARViewUpdatedViewport(viewPort);

    gARViewLayoutRequired = false;
    
    return (true);
}


// All tasks which require both video and OpenGL to be inited should be performed here.
// References globals: cameraLens, gCparamLT, gPixFormat
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

    // Load objects (i.e. OSG models).
    VirtualEnvironmentInit(objectDataFilename);
    VirtualEnvironmentHandleARViewUpdatedCameraLens(cameraLens);
    
    gARViewInited = true;
#ifdef DEBUG
    LOGI("ARView initialised.\n");
#endif

    return (true);
}

JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDrawFrame(JNIEnv* env, jobject obj))
{
	float width, height;
    
    if (!videoInited) {
#ifdef DEBUG
        LOGI("nativeDrawFrame !VIDEO\n");
#endif        
        return; // No point in trying to draw until video is inited.
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
        arglPixelBufferDataUploadBiPlanar(gArglSettings, gVideoFrame, gVideoFrame + videoWidth*videoHeight);
        videoFrameNeedsPixelBufferDataUpload = false;
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
    
    VirtualEnvironmentHandleARViewDrawPreCamera();
    
    if (cameraPoseValid) {
        
        glMultMatrixf(cameraPose);
        
        // All lighting and geometry to be drawn in world coordinates goes here.
        // --->
        VirtualEnvironmentHandleARViewDrawPostCamera();
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
    
    VirtualEnvironmentHandleARViewDrawOverlay();
    
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
#endif
}
