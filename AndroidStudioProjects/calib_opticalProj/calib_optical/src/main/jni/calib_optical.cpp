/*
 *  calib_optical.cpp
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
 *  Copyright 2015 Daqri LLC. All Rights Reserved.
 *  Copyright 2012-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Philip Lamb
 */

// ============================================================================
//	Includes
// ============================================================================

#include "calib_optical.h"

#include <jni.h>
#include <android/log.h>
#include <unistd.h> // getcwd()
#include <stdlib.h> // malloc()
#include <pthread.h>
#include <time.h> // gmtime()/localtime()
#include <sys/time.h> // struct timeval, gettimeofday()
#include <GLES/gl.h>

//#include <AR/ar.h>
#include <AR/arMulti.h>
#include <AR/video.h>
#include <AR/gsub_es.h>
#include <AR/arFilterTransMat.h>
#include <ARUtil/thread_sub.h>

#include <Eden/EdenSurfaces.h>
#include <Eden/EdenGLFont.h>
#include <Eden/EdenMessage.h>

#include "flow.h"
#include "calc_optical.h"

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
#define  LOG_TAG    "calib_optical"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


#define FONT_SIZE 5.0f


// ============================================================================
//	Global variables
// ============================================================================

// Main state.
static struct timeval gStartTime;

// Preferences.
static const char patt_name[]  = "Data/calib.patt";

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

// Marker detection.
static ARHandle		*gARHandle = NULL;
static ARPattHandle	*gARPattHandle = NULL;
static long			gCallCountMarkerDetect = 0;

// Transformation matrix retrieval.
static AR3DHandle	*gAR3DHandle = NULL;
static ARdouble     gPatt_width     = 80.0;	// Per-marker, but we are using only 1 marker.
static ARdouble     gPatt_trans[3][4];		// Per-marker, but we are using only 1 marker.
static int			gPatt_found = FALSE;	// Per-marker, but we are using only 1 marker.
static int			gPatt_id;				// Per-marker, but we are using only 1 marker.

// Drawing.
static ARParamLT *gCparamLT = NULL;
static int gVideoSeeThrough = 0;

#undef STEREO_SUPPORT_MODES_REQUIRING_STENCIL // Not enabled as full-window stencilling is very slow on most hardware.

// Stereo display mode. Not all modes may be supported in this application.
typedef enum {
    STEREO_DISPLAY_MODE_INACTIVE = 0,           // Stereo display not active.
    STEREO_DISPLAY_MODE_DUAL_OUTPUT,            // Two outputs, one displaying the left view, and one the right view.  Blue-line optional.
    STEREO_DISPLAY_MODE_QUADBUFFERED,           // One output exposing both left and right buffers, with display mode determined by the hardware implementation. Blue-line optional.
    STEREO_DISPLAY_MODE_FRAME_SEQUENTIAL,       // One output, first frame displaying the left view, and the next frame the right view. Blue-line optional.
    STEREO_DISPLAY_MODE_SIDE_BY_SIDE,           // One output. Two normally-proportioned views are drawn in the left and right halves.
    STEREO_DISPLAY_MODE_OVER_UNDER,             // One output. Two normally-proportioned views are drawn in the top and bottom halves.
    STEREO_DISPLAY_MODE_HALF_SIDE_BY_SIDE,      // One output. Two views, scaled to half-width, are drawn in the left and right halves
    STEREO_DISPLAY_MODE_OVER_UNDER_HALF_HEIGHT, // One output. Two views, scaled to half-height, are drawn in the top and bottom halves.
    STEREO_DISPLAY_MODE_ROW_INTERLACED,         // One output. Two views, normally proportioned, are interlaced, with even numbered rows drawn from the first view and odd numbered rows drawn from the second view.
    STEREO_DISPLAY_MODE_COLUMN_INTERLACED,      // One output. Two views, normally proportioned, are interlaced, with even numbered columns drawn from the first view and odd numbered columns drawn from the second view.
    STEREO_DISPLAY_MODE_CHECKERBOARD,           // One output. Two views, normally proportioned, are hatched. On even numbered rows, even numbered columns are drawn from the first view and odd numbered columns drawn from the second view. On odd numbered rows, this is reversed.
    STEREO_DISPLAY_MODE_ANAGLYPH_RED_BLUE,      // One output. Both views are rendered into the same buffer, the left view drawn only in the red channel and the right view only in the blue channel.
    STEREO_DISPLAY_MODE_ANAGLYPH_RED_GREEN,     // One output. Both views are rendered into the same buffer, the left view drawn only in the red channel and the right view only in the green channel.
} STEREO_DISPLAY_MODE;

static STEREO_DISPLAY_MODE stereoDisplayMode = STEREO_DISPLAY_MODE_INACTIVE;
static VIEW_EYE_t stereoDisplaySequentialNext = VIEW_LEFTEYE; // For frame-sequential output, even vs. odd frame.
static int stereoDisplayUseBlueLine = FALSE;
static int stereoDisplayReverseLeftRight = FALSE;
#ifdef STEREO_SUPPORT_MODES_REQUIRING_STENCIL
static unsigned char *stereoStencilPattern = NULL; // For modes that required stencilling, this will point to the stencil pattern.
#endif

enum viewPortIndices {
    viewPortIndexLeft = 0,
    viewPortIndexBottom,
    viewPortIndexWidth,
    viewPortIndexHeight
};

typedef struct {
    VIEW_EYE_t viewEye; // Set in main().
    GLenum drawBuffer; // Set in main().
    float contentWidth; // Set in Reshape(). This is the "true" shape of the content.
    float contentHeight; // Set in Reshape(). This is the "true" shape of the content.
    GLint viewPort[4]; // Set in Reshape(). Note that the width and height of the viewport may be different from that of the content.
} VIEW_t;

static int viewCount;
static VIEW_t *views;

typedef struct {
    int contextIndex;       // An index number by which this context is referred to. For GLUT-managed contexts, this is the GLUT window index.
    int width;              // Width in pixels of this context. Note that this may not be the same as the width of any referred-to view(s).
    int height;             // Height in pixels of this context. Note that this may not be the same as the height of any referred-to view(s).
    int viewCount;          // Number of views referred to by this context.
    VIEW_t **views;         // Reference(s) to view(s). Not owned, and should not be de de-alloced when this struct is dealloced.
    ARGL_CONTEXT_SETTINGS_REF arglSettings; // Per-context ARGL settings.
} VIEW_CONTEXT_t;

static int viewContextCount = 0;
static VIEW_CONTEXT_t *viewContexts;

// Calibration params.

static int gCalibrationEyeSelection = 0; // left.
static VIEW_EYE_t gCalibrationEye = VIEW_LEFTEYE;
static int co1 = -1; // Index into which x-y coordinate we are capturing.
static int co2 = -1; // Index into which z coordinate we are capturing.
#define  CALIB_POS1_NUM     5	// Use 5 positions in the display image plane (x-y)
#define  CALIB_POS2_NUM     2	// Use 2 positions in the display depth axis (z)
// Centre of the four quadrants, and centre of whole image.
static ARdouble	calib_pos[CALIB_POS1_NUM][2];
static ARdouble   calib_pos2d[CALIB_POS1_NUM][CALIB_POS2_NUM][2];
static ARdouble   calib_pos3d[CALIB_POS1_NUM][CALIB_POS2_NUM][3];


// Drawing.
static int backingWidth;
static int backingHeight;
static bool gARViewLayoutRequired = false;
static bool gARViewInited = false;
static TEXTURE_INDEX_t gSplashScreenTexture = 0u;

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
//	Android NDK function signatures
// ============================================================================

// Utility preprocessor directive so only one change needed if Java class name changes
// _1 is the escape sequence for a '_' character.
#define JNIFUNCTION_NATIVE(sig) Java_org_artoolkit_ar_utils_calib_1optical_calib_1optical_1Activity_##sig


extern "C" {
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeCreate(JNIEnv* env, jobject object, jobject instanceOfAndroidContext));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeStart(JNIEnv* env, jobject object));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeStop(JNIEnv* env, jobject object));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeDestroy(JNIEnv* env, jobject object));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeVideoInit(JNIEnv* env, jobject object, jint w, jint h, jint cameraIndex, jboolean cameraIsFrontFacing));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeVideoFrame(JNIEnv* env, jobject obj, jbyteArray pinArray)) ;
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSurfaceCreated(JNIEnv* env, jobject object));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSurfaceChanged(JNIEnv* env, jobject object, jint w, jint h));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDisplayParametersChanged(JNIEnv* env, jobject object, jint orientation, jint width, jint height, jint dpi, jint stereoDisplayMode));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDrawFrame(JNIEnv* env, jobject obj));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeHandleTouchAtLocation(JNIEnv* env, jobject object, jint x, jint y));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeHandleBackButton(JNIEnv* env, jobject object));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeHandleMenuToggleVideoSeeThrough(JNIEnv* env, jobject object));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeSetCalibrationParameters(JNIEnv* env, jobject object, jint eyeSelection));
};

// ============================================================================
//	Function prototypes
// ============================================================================

static void nativeVideoGetCparamCallback(const ARParam *cparam_p, void *userdata);

// ============================================================================
//	Functions
// ============================================================================

//
// Lifecycle functions.
// See http://developer.android.com/reference/android/app/Activity.html#ActivityLifecycle
//

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeCreate(JNIEnv* env, jobject object, jobject instanceOfAndroidContext/*, jstring configString*/))
{
    int err_i;
#ifdef DEBUG
    LOGI("nativeCreate\n");
#endif
    
    /*if (configString != NULL) {
        const char* configStringC = env->GetStringUTFChars(configString, NULL);
        
        //
        
        env->ReleaseStringUTFChars(configString, configStringC);
    }*/
    /*
                if      (strcmp(argv[i], "INACTIVE") == 0)                  stereoDisplayMode = STEREO_DISPLAY_MODE_INACTIVE;
                else if (strcmp(argv[i], "DUAL_OUTPUT") == 0)               stereoDisplayMode = STEREO_DISPLAY_MODE_DUAL_OUTPUT;
                else if (strcmp(argv[i], "QUADBUFFERED") == 0)              stereoDisplayMode = STEREO_DISPLAY_MODE_QUADBUFFERED;
                else if (strcmp(argv[i], "FRAME_SEQUENTIAL") == 0)          stereoDisplayMode = STEREO_DISPLAY_MODE_FRAME_SEQUENTIAL;
                else if (strcmp(argv[i], "SIDE_BY_SIDE") == 0)              stereoDisplayMode = STEREO_DISPLAY_MODE_SIDE_BY_SIDE;
                else if (strcmp(argv[i], "OVER_UNDER") == 0)                stereoDisplayMode = STEREO_DISPLAY_MODE_OVER_UNDER;
                else if (strcmp(argv[i], "HALF_SIDE_BY_SIDE") == 0)         stereoDisplayMode = STEREO_DISPLAY_MODE_HALF_SIDE_BY_SIDE;
                else if (strcmp(argv[i], "OVER_UNDER_HALF_HEIGHT") == 0)    stereoDisplayMode = STEREO_DISPLAY_MODE_OVER_UNDER_HALF_HEIGHT;
                else if (strcmp(argv[i], "ANAGLYPH_RED_BLUE") == 0)         stereoDisplayMode = STEREO_DISPLAY_MODE_ANAGLYPH_RED_BLUE;
                else if (strcmp(argv[i], "ANAGLYPH_RED_GREEN") == 0)        stereoDisplayMode = STEREO_DISPLAY_MODE_ANAGLYPH_RED_GREEN;
                else if (strcmp(argv[i], "ROW_INTERLACED") == 0)            stereoDisplayMode = STEREO_DISPLAY_MODE_ROW_INTERLACED;
                else if (strcmp(argv[i], "COLUMN_INTERLACED") == 0)         stereoDisplayMode = STEREO_DISPLAY_MODE_COLUMN_INTERLACED;
                else if (strcmp(argv[i], "CHECKERBOARD") == 0)              stereoDisplayMode = STEREO_DISPLAY_MODE_CHECKERBOARD;
    */
    
    // Change working directory for the native process, so relative paths can be used for file access.
    arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST, NULL, instanceOfAndroidContext);

    // Load marker(s).
    gARPattHandle = arPattCreateHandle();
	if (gARPattHandle == NULL) {
		LOGE("Error creating pattern handle");
		return false;
	}

	// Loading pattern.
	if ((gPatt_id = arPattLoad(gARPattHandle, patt_name)) < 0) {
		LOGE("Error loading pattern file %s.\n", patt_name);
		arPattDeleteHandle(gARPattHandle);
		gARPattHandle = NULL;
		return false;
	}


	return (true);
}

// Settings functions, may be called prior to nativeStart.

// 0 = portrait, 1 = landscape (device rotated 90 degrees ccw), 2 = portrait upside down, 3 = landscape reverse (device rotated 90 degrees cw).
JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDisplayParametersChanged(JNIEnv* env, jobject object, jint orientation, jint width, jint height, jint dpi, jint stereoDisplayMode_in))
{
    LOGI("nativeDisplayParametersChanged orientation=%d, size=%dx%d@%dpi, stereoDisplayMode=%d\n", orientation, width, height, dpi, stereoDisplayMode_in);
	gDisplayOrientation = orientation;
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
	gDisplayWidth = width;
	gDisplayHeight = height;
	gDisplayDPI = dpi;
	stereoDisplayMode = (STEREO_DISPLAY_MODE)stereoDisplayMode_in;

	EdenGLFontSetDisplayResolution((float)dpi);

    gARViewLayoutRequired = true;
}


JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeSetCalibrationParameters(JNIEnv* env, jobject object, jint eyeSelection))
{
    LOGI("nativeSetCalibrationParameters eyeSelection=%d\n", eyeSelection);

    gCalibrationEyeSelection = eyeSelection;

    return true;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeStart(JNIEnv* env, jobject object))
{
#ifdef DEBUG
    LOGI("nativeStart\n");
#endif
    // Set up views.
 	viewCount = (stereoDisplayMode == STEREO_DISPLAY_MODE_INACTIVE ? 1 : 2);
    arMallocClear(views, VIEW_t, viewCount);

    // Configure contexts.
    if (stereoDisplayMode == STEREO_DISPLAY_MODE_DUAL_OUTPUT) {
        // dual-output stereo.
        viewContextCount = 2;
        arMallocClear(viewContexts, VIEW_CONTEXT_t, viewContextCount);
        viewContexts[0].viewCount = 1;
        arMallocClear(viewContexts[0].views, VIEW_t *, viewContexts[0].viewCount);
        viewContexts[0].views[0] = &(views[0]);
        viewContexts[0].views[0]->viewEye = (!stereoDisplayReverseLeftRight ? VIEW_LEFTEYE : VIEW_RIGHTEYE); // Left first, unless reversed.
        viewContexts[0].views[0]->drawBuffer = GL_BACK;
        viewContexts[1].viewCount = 1;
        arMallocClear(viewContexts[1].views, VIEW_t *, viewContexts[1].viewCount);
        viewContexts[1].views[0] = &(views[1]);
        viewContexts[1].views[0]->viewEye = (!stereoDisplayReverseLeftRight ? VIEW_RIGHTEYE : VIEW_LEFTEYE); // Right second, unless reversed.
        viewContexts[1].views[0]->drawBuffer = GL_BACK;
    } else {
        // single-output.
        viewContextCount = 1;
        arMallocClear(viewContexts, VIEW_CONTEXT_t, viewContextCount);
        if (stereoDisplayMode == STEREO_DISPLAY_MODE_INACTIVE) {
            // mono.
            viewContexts[0].viewCount = 1;
            arMallocClear(viewContexts[0].views, VIEW_t *, viewContexts[0].viewCount);
            viewContexts[0].views[0] = &(views[0]);
            viewContexts[0].views[0]->viewEye = VIEW_LEFTEYE;
            viewContexts[0].views[0]->drawBuffer = GL_BACK;
        } else {
            // stereo.
            viewContexts[0].viewCount = 2;
            arMallocClear(viewContexts[0].views, VIEW_t *, viewContexts[0].viewCount);
            viewContexts[0].views[0] = &(views[0]);
            viewContexts[0].views[0]->viewEye = (!stereoDisplayReverseLeftRight ? VIEW_LEFTEYE : VIEW_RIGHTEYE); // Left first, unless reversed.
            viewContexts[0].views[0]->drawBuffer = ( /*stereoDisplayMode == STEREO_DISPLAY_MODE_QUADBUFFERED ? GL_BACK_LEFT : */ GL_BACK);
            viewContexts[0].views[1] = &(views[1]);
            viewContexts[0].views[1]->viewEye = (!stereoDisplayReverseLeftRight ? VIEW_RIGHTEYE : VIEW_LEFTEYE); // Right second, unless reversed.
            viewContexts[0].views[1]->drawBuffer = ( /*stereoDisplayMode == STEREO_DISPLAY_MODE_QUADBUFFERED ? GL_BACK_RIGHT : */ GL_BACK);
        }
    }


    // Library setup.
    EdenSurfacesInit(viewContextCount, 16); // Allow space for 16 textures.
    EdenGLFontInit(viewContextCount);
    EdenGLFontSetSize(FONT_SIZE);
    EdenGLFontSetFont(EDEN_GL_FONT_ID_Stroke_Roman);
    EdenGLFontSetWordSpacing(0.8f);
    EdenMessageInit(viewContextCount);

	if (!flowInitAndStart(stereoDisplayMode != STEREO_DISPLAY_MODE_INACTIVE, gCalibrationEyeSelection)) {
		return (false);
    }

    // Get start time.
    gettimeofday(&gStartTime, NULL);

    gVid = ar2VideoOpen("");
    if (!gVid) {
    	LOGE("Error: ar2VideoOpen.\n");
    	return (false);
    }

    // Since most AR init can't be completed until the video frame size is known,
    // that will be deferred.
    
    // Also, GL init depends on having an OpenGL context.
    
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
    
    // Clean up ARToolKit data.
    if (gAR3DHandle) ar3DDeleteHandle(&gAR3DHandle);
    if (gARHandle) {
        arPattDetach(gARHandle);
        arDeleteHandle(gARHandle);
        gARHandle = NULL;
    }
    arParamLTFree(&gCparamLT);

    // Can't call arglCleanup() here, because nativeStop is not called on rendering thread.

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
    if (gVid) {
        ar2VideoClose(gVid);
        gVid = NULL;
    }
    videoInited = false;

    EdenMessageFinal();
    EdenGLFontFinal();
    EdenSurfacesFinal();

#ifdef STEREO_SUPPORT_MODES_REQUIRING_STENCIL
    if (stereoStencilPattern) free(stereoStencilPattern);
#endif

    if (viewContexts) {
        for (i = 0; i < viewContextCount; i++) {
            if (viewContexts[i].arglSettings) arglCleanup(viewContexts[i].arglSettings); // FIXME: Needs an active OpenGL thread.
            free(viewContexts[i].views);
        }
        free(viewContexts);
        viewContextCount = 0;
    }
    if (views) {
        free(views);
        viewCount = 0;
    }

    flowStopAndFinal();

    return (true);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeDestroy(JNIEnv* env, jobject object))
{
#ifdef DEBUG
    LOGI("nativeDestroy\n");
#endif
    int i;

    if (gARPattHandle) {
        arPattDeleteHandle(gARPattHandle);
        gARPattHandle = NULL;
    }

	return (true);
}

//static void quit(JNIEnv* env, jobject object)
//{
//	jclass classOfCaller = env->GetObjectClass(object);
//	if (!classOfCaller) return;
//	jmethodID methodFinishFromNative = env->GetMethodID(classOfCaller, "finishFromNative", "()V");
//	if (!methodFinishFromNative) return;
//	env->CallVoidMethod(object, methodFinishFromNative);
//}

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

    // Init AR.
    gARHandle = arCreateHandle(gCparamLT);
    if (gARHandle == NULL) {
        LOGE("Error creating AR handle");
        return;
    }
    arPattAttach(gARHandle, gARPattHandle);

    if (arSetPixelFormat(gARHandle, gPixFormat) < 0) {
        LOGE("Error setting pixel format");
        return;
    }

    gAR3DHandle = ar3DCreateHandle(&gCparamLT->param);
    if (gAR3DHandle == NULL) {
        LOGE("Error creating 3D handle");
        return;
    }

    //
    // Other ARToolKit setup.
    //

    //arSetMarkerExtractionMode(gARHandle, AR_USE_TRACKING_HISTORY_V2);
    arSetMarkerExtractionMode(gARHandle, AR_NOUSE_TRACKING_HISTORY);
    //arSetLabelingThreshMode(gARHandle, AR_LABELING_THRESH_MODE_MANUAL); // Uncomment to use  manual thresholding.
}

JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeVideoFrame(JNIEnv* env, jobject obj, jbyteArray pinArray))
{
    int i, j, k;
    jbyte* inArray;
    ARdouble err;
    int cornerFlag;
        
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
    //LOGI("nativeVideoFrame\n");
#endif        
    
    // Copy the incoming  YUV420 image in pinArray.
    env->GetByteArrayRegion(pinArray, 0, gVideoFrameSize, (jbyte *)gVideoFrame);
    
	// As of ARToolKit v5.0, NV21 format video frames are handled natively,
	// and no longer require colour conversion to RGBA.
	// If you still require RGBA format information from the video,
    // here is where you'd do the conversion:
    // color_convert_common(gVideoFrame, gVideoFrame + videoWidth*videoHeight, videoWidth, videoHeight, myRGBABuffer);

    videoFrameNeedsPixelBufferDataUpload = true; // Note that buffer needs uploading. (Upload must be done on OpenGL context's thread.)

	gCallCountMarkerDetect++; // Increment ARToolKit FPS counter.

	// Detect the markers in the video frame.
	if (arDetectMarker(gARHandle, gVideoBuffer) < 0) {
		exit(-1);
	}

	// Check through the marker_info array for highest confidence
	// visible marker matching our preferred pattern.
	k = -1;
	for (j = 0; j < gARHandle->marker_num; j++) {
		if (gARHandle->markerInfo[j].id == gPatt_id) {
			if (k == -1) k = j; // First marker detected.
			else if (gARHandle->markerInfo[j].cf > gARHandle->markerInfo[k].cf) k = j; // Higher confidence marker detected.
		}
	}

	if (k != -1) {
		// Get the transformation between the marker and the real camera into gPatt_trans.
		err = arGetTransMatSquare(gAR3DHandle, &(gARHandle->markerInfo[k]), gPatt_width, gPatt_trans);
#ifdef DEBUG
		if (!gPatt_found) LOGI("Found calibration target.\n");
#endif
		gPatt_found = TRUE;
	} else {
#ifdef DEBUG
		if (gPatt_found) LOGI("Lost calibration target.\n");
#endif
		gPatt_found = FALSE;
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

	int contextIndex = 0;
	if (viewContexts[contextIndex].arglSettings) {
		arglCleanup(viewContexts[contextIndex].arglSettings); // Clean up any left-over ARGL data.
		viewContexts[contextIndex].arglSettings = NULL;
	}
	if (gSplashScreenTexture) EdenSurfacesTextureUnload(contextIndex, 1, &gSplashScreenTexture); // Clean up left-over EdenSurfaces data.

    gARViewInited = false;

    // Now create the contexts.
    viewContexts[contextIndex].contextIndex = contextIndex;

}

//
// This is called when something about the surface changes. e.g. size.
//
// Modifies globals: backingWidth, backingHeight, gARViewLayoutRequired.
JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSurfaceChanged(JNIEnv* env, jobject object, jint w, jint h))
{
    int contextIndex;
    backingWidth = w;
    backingHeight = h;
    LOGI("nativeSurfaceChanged w=%d, h=%d\n", w, h);
    
	// Call through to anyone else who needs to know about window sizing here.

    // In order to do something meaningful with the surface backing size in an AR sense,
    // we also need the content size, which we aren't guaranteed to have yet, so defer
    // the viewPort calculations.
    gARViewLayoutRequired = true;
}

// Lays out the AR view. Requires both video and OpenGL to be inited, and must be called on OpenGL thread.
// References globals: gContentMode, backingWidth, backingHeight, videoWidth, videoHeight, gContentRotate90.
// Modifies globals: viewPort, gARViewLayoutRequired.
static bool layoutARView(const int contextIndex)
{

	//arglSetRotate90(gArglSettings, gContentRotate90);
    //arglSetFlipV(gArglSettings, gContentFlipV);
    //arglSetFlipH(gArglSettings, gContentFlipH);

    int viewPortWidth, viewPortHeight;
    float contentWidth, contentHeight;
    int viewIndex;
    //int r, c;

    // Assuming a single context.
    int w = backingWidth;
    int h = backingHeight;
    if (contextIndex >= 0 && contextIndex < viewContextCount) {
        viewContexts[contextIndex].width = w;
        viewContexts[contextIndex].height = h;
    }

    //
    // Determine the content size. This determines the proportions
    // of the OpenGL viewing frustum and any orthographic view.
    //
    // For an optical display, we assume that the window size is equal
    // to the display size, and that the display has square pixels.
    // So, we can just adopt the window size as content size, or
    // for the non-half split, half the split dimension.
    //
    if (stereoDisplayMode == STEREO_DISPLAY_MODE_SIDE_BY_SIDE) {
        contentWidth = (float)(w / 2);
    } else {
        contentWidth = (float)w;
    }
    if (stereoDisplayMode == STEREO_DISPLAY_MODE_OVER_UNDER) {
        contentHeight = (float)(h / 2);
    } else {
        contentHeight = (float)h;
    }

    // Calculate viewport(s).
    if (stereoDisplayMode == STEREO_DISPLAY_MODE_SIDE_BY_SIDE || stereoDisplayMode == STEREO_DISPLAY_MODE_HALF_SIDE_BY_SIDE) {
        viewPortWidth = w / 2;
    } else {
        viewPortWidth = w;
    }
    if (stereoDisplayMode == STEREO_DISPLAY_MODE_OVER_UNDER || stereoDisplayMode == STEREO_DISPLAY_MODE_OVER_UNDER_HALF_HEIGHT) {
        viewPortHeight = h / 2;
    } else {
        viewPortHeight = h;
    }

    for (viewIndex = 0; viewIndex < (stereoDisplayMode == STEREO_DISPLAY_MODE_INACTIVE ? 1 : 2); viewIndex++) {
        views[viewIndex].contentWidth = contentWidth;
        views[viewIndex].contentHeight = contentHeight;
        if ((stereoDisplayMode == STEREO_DISPLAY_MODE_SIDE_BY_SIDE || stereoDisplayMode == STEREO_DISPLAY_MODE_HALF_SIDE_BY_SIDE) && viewIndex == 1)
            views[viewIndex].viewPort[viewPortIndexLeft] = viewPortWidth;
        else
            views[viewIndex].viewPort[viewPortIndexLeft] = 0;
        if ((stereoDisplayMode == STEREO_DISPLAY_MODE_OVER_UNDER || stereoDisplayMode == STEREO_DISPLAY_MODE_OVER_UNDER_HALF_HEIGHT) && viewIndex == 0)
            views[viewIndex].viewPort[viewPortIndexBottom] = viewPortHeight;
        else
            views[viewIndex].viewPort[viewPortIndexBottom] = 0;
        views[viewIndex].viewPort[viewPortIndexWidth] = viewPortWidth;
        views[viewIndex].viewPort[viewPortIndexHeight] = viewPortHeight;
    }

#ifdef STEREO_SUPPORT_MODES_REQUIRING_STENCIL
    // If the modes requires stencilling, create the stencils now.
    if (stereoDisplayMode == STEREO_DISPLAY_MODE_ROW_INTERLACED || stereoDisplayMode == STEREO_DISPLAY_MODE_COLUMN_INTERLACED || stereoDisplayMode == STEREO_DISPLAY_MODE_CHECKERBOARD) {
        if (stereoStencilPattern) {
            free(stereoStencilPattern);
            stereoStencilPattern = NULL;
        }
        stereoStencilPattern = (unsigned char *)valloc(w*h * sizeof(unsigned char));
        if (!stereoStencilPattern) {
            ARLOGe("Out of memory!!\n");
            exit (-1);
        }
        if (stereoDisplayMode == STEREO_DISPLAY_MODE_ROW_INTERLACED) {
            for (r = 0; r < h; r++) {
                memset(stereoStencilPattern + r*w, !(r&1), w);
            }
        } else if (stereoDisplayMode == STEREO_DISPLAY_MODE_COLUMN_INTERLACED) {
            for (r = 0; r < h; r++) {
                for (c = 0; c < w; c++) {
                    stereoStencilPattern[r*w + c] = !(c&1);
                }
            }
        } else { // (stereoDisplayMode == STEREO_DISPLAY_MODE_CHECKERBOARD)
            for (r = 0; r < h; r++) {
                for (c = 0; c < w; c++) {
                    stereoStencilPattern[r*w + c] = !(c&1 ^ r&1);
                }
            }
        }
    }
#endif

	// Call through to anyone else who needs to know about window sizing here.
    EdenGLFontSetViewSize(contentWidth, contentHeight);
    EdenMessageSetViewSize(contentWidth, contentHeight);

    gARViewLayoutRequired = false;
    
    return (true);
}


// All tasks which require both video and OpenGL to be inited should be performed here.
// References globals: gPixFormat
// Modifies globals: gArglSettings, gArglSettingsCornerFinderImage
static bool initARView(const int contextIndex)
{
#ifdef DEBUG
    LOGI("Initialising ARView\n");
#endif        
    if (gARViewInited) return (false);
    
#ifdef DEBUG
    LOGI("Setting up argl.\n");
#endif
    // Setup a route for rendering the background image.
    if ((viewContexts[contextIndex].arglSettings = arglSetupForCurrentContext(&(gCparamLT->param), gPixFormat)) == NULL) {
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

static void drawBackground(const float width, const float height, const float x, const float y, const bool drawBorder)
{
    GLfloat vertices[4][2];

    vertices[0][0] = x; vertices[0][1] = y;
    vertices[1][0] = width + x; vertices[1][1] = y;
    vertices[2][0] = width + x; vertices[2][1] = height + y;
    vertices[3][0] = x; vertices[3][1] = height + y;

    glLoadIdentity();
    glStateCacheDisableDepthTest();
    glStateCacheDisableLighting();
    glStateCacheDisableTex2D();
    glStateCacheBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glStateCacheEnableBlend();
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glStateCacheEnableClientStateVertexArray();
    glStateCacheDisableClientStateNormalArray();
    glStateCacheClientActiveTexture(GL_TEXTURE0);
    glStateCacheDisableClientStateTexCoordArray();
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);	// 50% transparent black.
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    if (drawBorder) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Opaque white.
        glLineWidth(1.0f);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }
}

static void drawLineSeg(float x1, float y1, float x2, float y2)
{
	float vertices[2][2];
    vertices[0][0] = x1;
    vertices[0][1] = y1;
    vertices[1][0] = x2;
    vertices[1][1] = y2;
    
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glStateCacheEnableClientStateVertexArray();
    glStateCacheClientActiveTexture(GL_TEXTURE0);
    glStateCacheDisableClientStateTexCoordArray();
    glStateCacheDisableClientStateNormalArray();
    glDrawArrays(GL_LINES, 0, 2);
}

static int drawAttention(const float posx, const float posy, const float side, const int color)
{
	float sided2 = side / 2.0f;
	
    switch(color%7) {
		case 0: glColor4ub(255, 0, 0, 255); break;
		case 1: glColor4ub(0, 255, 0, 255); break;
		case 2: glColor4ub(0, 0, 255, 255); break;
		case 3: glColor4ub(255, 255, 0, 255); break;
		case 4: glColor4ub(255, 0, 255, 255); break;
		case 5: glColor4ub(0, 255, 255, 255); break;
		case 6: glColor4ub(255, 255, 255, 255); break;
    }
	
    glLineWidth(5.0f);
    drawLineSeg(posx - sided2, posy - sided2, posx + sided2, posy - sided2);
    drawLineSeg(posx - sided2, posy + sided2, posx + sided2, posy + sided2);
    drawLineSeg(posx - sided2, posy - sided2, posx - sided2, posy + sided2);
    drawLineSeg(posx + sided2, posy - sided2, posx + sided2, posy + sided2);
    glLineWidth(1.0f);
	
    return(0);
}

#if 0
static int findContextIndex(void)
{
    int i, window;
    
    if (glutGameModeGet(GLUT_GAME_MODE_ACTIVE)) {
		return (0);
	} else {
		// Linear search through all active contexts to find context index for this glut window, calling DisplayPerContext() if found.
		window = glutGetWindow();
		for (i = 0; i < viewContextCount; i++) {
			if (viewContexts[i].contextIndex == window) break;
		}
		if (i < viewContextCount) {
			return (i);
		} else {
            return (-1);
        }
	}
}
#endif

JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDrawFrame(JNIEnv* env, jobject obj))
{
	int contextIndex;
    int i;
    VIEW_t *view;
    VIEW_EYE_t viewEye;
    float left, right, bottom, top;
	GLfloat *vertices;
	GLint vertexCount;
    
#if 0
	contextIndex = findContextIndex();
    if (contextIndex >= 0 && contextIndex < viewContextCount) {
		DisplayPerContext(contextIndex);
		glutSwapBuffers();
    }
#else
	// Assuming a single context.
	contextIndex = 0;
#endif

	if (!videoInited) {
#ifdef DEBUG
    	LOGI("nativeDrawFrame !VIDEO\n");
#endif
        // Set up the background texture for the loading screen.
        if (!gSplashScreenTexture) {
	        const TEXTURE_INFO_t ti = {(backingWidth > backingHeight ? "Data/artoolkit_logo_landscape_720p.jpg" : "Data/artoolkit_logo_portrait_720p.jpg"), GL_FALSE, GL_RGB, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0.0f, GL_REPLACE};
            if (!EdenSurfacesTextureLoad(contextIndex, 1, &ti, &gSplashScreenTexture, NULL)) {
                LOGE("Unable to load texture '%s'.\n", ti.pathname);
            }
        }

        // Draw splash screen.
        glViewport(0, 0, backingWidth, backingHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrthof(0.0f, (float)backingWidth, 0.0f, (float)backingHeight, -1.0f, 1.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        if (gSplashScreenTexture) {
            EdenSurfacesDraw(0, gSplashScreenTexture, backingWidth, backingHeight, FILL, CENTRE);
        }
        return; // No point in trying to draw until video is inited.
    }

#ifdef DEBUG
    //LOGI("nativeDrawFrame\n");
#endif
    if (!gARViewInited) {
        if (!initARView(contextIndex)) return;
        if (gSplashScreenTexture) EdenSurfacesTextureUnload(contextIndex, 1, &gSplashScreenTexture);
    }
    if (gARViewLayoutRequired) layoutARView(contextIndex);





    // Clear the buffer(s) now.
    //glDrawBuffer(GL_BACK); // Includes both GL_BACK_LEFT and GL_BACK_RIGHT (if defined).
#ifdef STEREO_SUPPORT_MODES_REQUIRING_STENCIL
    if (stereoDisplayMode == STEREO_DISPLAY_MODE_ROW_INTERLACED || stereoDisplayMode == STEREO_DISPLAY_MODE_COLUMN_INTERLACED || stereoDisplayMode == STEREO_DISPLAY_MODE_CHECKERBOARD) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear the buffers for new frame.
        // Cover the stencil buffer for the entire context with the stencil pattern.
        glViewport(0, 0, viewContexts[contextIndex].width, viewContexts[contextIndex].height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, viewContexts[contextIndex].width, 0, viewContexts[contextIndex].height, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRasterPos2f(0.0f, 0.0f);
        glPixelTransferi(GL_UNPACK_ALIGNMENT, ((viewContexts[contextIndex].width & 0x3) == 0 ? 4 : 1));
        glPixelZoom(1.0f, 1.0f);
        glDrawPixels(viewContexts[contextIndex].width, viewContexts[contextIndex].height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stereoStencilPattern);
    } else
#endif
    if (stereoDisplayMode == STEREO_DISPLAY_MODE_ANAGLYPH_RED_BLUE || stereoDisplayMode == STEREO_DISPLAY_MODE_ANAGLYPH_RED_GREEN) {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers for new frame.
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers for new frame.
    }


	// This loop is once per view (i.e. once per eye).
	for (i = 0; i < viewContexts[contextIndex].viewCount; i++) {
        view = viewContexts[contextIndex].views[i];
        viewEye = view->viewEye;

        if (stereoDisplayMode == STEREO_DISPLAY_MODE_FRAME_SEQUENTIAL) {
            if (viewEye != stereoDisplaySequentialNext) continue;
        }

        // Select correct buffer for this context. (It has already been cleared.)
		//glDrawBuffer(view->drawBuffer);

#ifdef STEREO_SUPPORT_MODES_REQUIRING_STENCIL
        if (stereoDisplayMode == STEREO_DISPLAY_MODE_ROW_INTERLACED || stereoDisplayMode == STEREO_DISPLAY_MODE_COLUMN_INTERLACED || stereoDisplayMode == STEREO_DISPLAY_MODE_CHECKERBOARD) {
            // Stencil-based modes.
            glStencilFunc((viewEye == VIEW_LEFTEYE ? GL_EQUAL : GL_NOTEQUAL), 1, 1);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glEnable(GL_STENCIL_TEST);
        } else
#endif
        if (stereoDisplayMode == STEREO_DISPLAY_MODE_ANAGLYPH_RED_BLUE || stereoDisplayMode == STEREO_DISPLAY_MODE_ANAGLYPH_RED_GREEN ) {
            // Cheap red/blue or red/green anaglyph.
            if (i > 0) glClear(GL_DEPTH_BUFFER_BIT); // Second view is drawn into same colour buffer, just need to clear the depth buffer.
            if (viewEye == VIEW_LEFTEYE) glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
            else {
                if (stereoDisplayMode == STEREO_DISPLAY_MODE_ANAGLYPH_RED_BLUE) glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
                else glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
            }
        }

        glViewport(view->viewPort[viewPortIndexLeft], view->viewPort[viewPortIndexBottom], view->viewPort[viewPortIndexWidth], view->viewPort[viewPortIndexHeight]);

        if (gVideoSeeThrough) {
            // Upload new video frame if required.
            if (videoFrameNeedsPixelBufferDataUpload) {
                arglPixelBufferDataUploadBiPlanar(viewContexts[contextIndex].arglSettings, gVideoFrame, gVideoFrame + videoWidth*videoHeight);
                videoFrameNeedsPixelBufferDataUpload = false;
            }

            // Display the current frame
        	arglDispImage(viewContexts[contextIndex].arglSettings);
       }

        // Only 2D in this app.
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrthof(0.0f, view->contentWidth, 0.0f, view->contentHeight, -1.0f, 1.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glStateCacheDisableDepthTest();
        glStateCacheDisableLighting();
        glStateCacheDisableTex2D();

        if (flowStateGet() == FLOW_STATE_CAPTURING && viewEye == gCalibrationEye) {
            // Draw cross hairs.
            glLineWidth(3.0f);
            glColor4ub(255, 255, 255, 255);
            drawLineSeg(0.0f, (float)calib_pos[co1][1], (float)(view->contentWidth), (float)calib_pos[co1][1]);
            drawLineSeg((float)calib_pos[co1][0], 0.0f, (float)calib_pos[co1][0], (float)(view->contentHeight));
            glLineWidth(1.0f);

            // Draw box.
            if (gPatt_found) {
                drawAttention(calib_pos[co1][0], calib_pos[co1][1], 40.0, co2);
            }
        }

        //
        // Draw help text and mode.
        //
        glLoadIdentity();

		if (gPatt_found) {
			char *buf;
			asprintf(&buf, "(x, y z) = (%5f, %5f, %5f)", gPatt_trans[0][3], gPatt_trans[1][3], gPatt_trans[2][3]);
            EdenGLFontDrawLine(contextIndex, (unsigned char *)buf, 2.0f, 2.0f, H_OFFSET_TEXT_RIGHT_EDGE_TO_VIEW_RIGHT_EDGE, V_OFFSET_VIEW_TEXT_TOP_TO_VIEW_TOP);
            /*
            asprintf(&(lines[lineCount++]), "r (x, y z) = (%5f, %5f, %5f)", gPatt_trans[0][0], gPatt_trans[1][0], gPatt_trans[2][0]);
            asprintf(&(lines[lineCount++]), "u (x, y z) = (%5f, %5f, %5f)", gPatt_trans[0][1], gPatt_trans[1][1], gPatt_trans[2][1]);
            asprintf(&(lines[lineCount++]), "n (x, y z) = (%5f, %5f, %5f)", gPatt_trans[0][2], gPatt_trans[1][2], gPatt_trans[2][2]);
             */
		}

    	// Draw status bar with centred status message.
    	float statusBarHeight = EdenGLFontGetHeight() + 4.0f; // 2 pixels above, 2 below.
    	drawBackground(view->contentWidth, statusBarHeight, 0.0f, 0.0f, false);
    	glStateCacheDisableBlend();
    	glColor4ub(255, 255, 255, 255);
    	EdenGLFontDrawLine(contextIndex, statusBarMessage, 0.0f, 2.0f, H_OFFSET_VIEW_CENTER_TO_TEXT_CENTER, V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE);

    	// If a message should be onscreen, draw it.
    	if (gEdenMessageDrawRequired) {
    		glLoadIdentity();
    		EdenMessageDraw(contextIndex);
    	}

    	if (stereoDisplayUseBlueLine) {
        	GLfloat vertices[4][2];
            glViewport(view->viewPort[viewPortIndexLeft], view->viewPort[viewPortIndexBottom], view->viewPort[viewPortIndexWidth], view->viewPort[viewPortIndexHeight]);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrthof(0.0f, (GLfloat)view->viewPort[viewPortIndexWidth], 0.0f, (GLfloat)view->viewPort[viewPortIndexHeight], -1.0f, 1.0f);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glStateCacheDisableDepthTest();
            glStateCacheDisableBlend();
            glStateCacheDisableLighting();
            glStateCacheActiveTexture(GL_TEXTURE0);
            glStateCacheDisableTex2D();
            glLineWidth(1.0f);
            vertices[0][0] = 0.0f;
            vertices[0][1] = 0.5f;
            vertices[1][0] = (GLfloat)view->viewPort[viewPortIndexWidth];
            vertices[1][1] = 0.5f;
            vertices[2][0] = 0.0f;
            vertices[2][1] = 0.5f;
            vertices[3][0] = (GLfloat)view->viewPort[viewPortIndexWidth] * (viewEye == VIEW_LEFTEYE ? 0.3f : 0.8f);
            vertices[3][1] = 0.5f;
            glVertexPointer(2, GL_FLOAT, 0, vertices);
            glStateCacheEnableClientStateVertexArray();
            glStateCacheDisableClientStateNormalArray();
            glStateCacheClientActiveTexture(GL_TEXTURE0);
            glStateCacheDisableClientStateTexCoordArray();
            glColor4ub(0, 0, 0, 255);
            glDrawArrays(GL_LINES, 0, 2);
            glColor4ub(0, 0, 255, 255);
            glDrawArrays(GL_LINES, 2, 2);
        }

    } // end of view.

    if (stereoDisplayMode == STEREO_DISPLAY_MODE_FRAME_SEQUENTIAL) {
        stereoDisplaySequentialNext = (stereoDisplaySequentialNext == VIEW_LEFTEYE ? VIEW_RIGHTEYE : VIEW_LEFTEYE);
    }

    //if (gVideoSeeThrough) gARTImage = NULL; // All views done, can invalidate image data now.









}

JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeHandleTouchAtLocation(JNIEnv* env, jobject object, jint x, jint y))
{
	flowHandleEvent(EVENT_TOUCH);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeHandleBackButton(JNIEnv* env, jobject object))
{
	return ((jboolean)flowHandleEvent(EVENT_BACK_BUTTON));
}

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeHandleMenuToggleVideoSeeThrough(JNIEnv* env, jobject object))
{
	gVideoSeeThrough = !gVideoSeeThrough;
	return true;
}

bool captureInit(const VIEW_EYE_t calibrationEye)
{
	int i;
    int viewIndex;
    ARdouble contentWidth, contentHeight;
	const ARdouble calib_posn_norm[CALIB_POS1_NUM][2] = {
		{ 0.25, 0.25 },
		{ 0.75, 0.25 },
		{ 0.50, 0.50 },
		{ 0.25, 0.75 },
		{ 0.75, 0.75 }
	};

	gCalibrationEye = calibrationEye;

	// Adjust the on-screen calibration positions from normalised positions to the actual size we have.
    for (viewIndex = 0; views[viewIndex].viewEye != gCalibrationEye; viewIndex++); // Locate the view we're calibrating.
    contentWidth = (ARdouble)(views[viewIndex].contentWidth);
    contentHeight = (ARdouble)(views[viewIndex].contentHeight);
	for (i = 0; i < CALIB_POS1_NUM; i++) {
		calib_pos[i][0] = calib_posn_norm[i][0] * contentWidth;
		calib_pos[i][1] = calib_posn_norm[i][1] * contentHeight;
		LOGI("calib_pos[%d] = {%f, %f}\n", i, calib_pos[i][0], calib_pos[i][1]);
	}

	co1 = co2 = 0;

	return true;
}

bool capture(int *co1_p, int *co2_p)
{
	bool ret = false;

	if (co1 >= 0 && co1 < CALIB_POS1_NUM && co2 >= 0 && co2 < CALIB_POS2_NUM) {

		if (gPatt_found) {
			ARLOG("Position %d (%s) captured.\n", co1 + 1, (co2 == 1 ? "near" : "far"));
			ARLOG("-- 3D position %5f, %5f, %5f.\n", gPatt_trans[0][3], gPatt_trans[1][3], gPatt_trans[2][3]);
			ARLOG("-- 2D position %5f, %5f.\n", calib_pos[co1][0], calib_pos[co1][1]);
			calib_pos3d[co1][co2][0] = gPatt_trans[0][3];
			calib_pos3d[co1][co2][1] = gPatt_trans[1][3];
			calib_pos3d[co1][co2][2] = gPatt_trans[2][3];
			calib_pos2d[co1][co2][0] = calib_pos[co1][0];
			calib_pos2d[co1][co2][1] = calib_pos[co1][1];

			co2++;
			if (co2 >= CALIB_POS2_NUM) {
				co2 = 0;
				co1++;
				if (co1 >= CALIB_POS1_NUM) ret = true;
			}
		}
	}

	*co1_p = co1;
	*co2_p = co2;
	return ret;
}

bool captureUndo(int *co1_p, int *co2_p)
{
	bool ret = false;

	if (co1 >= 0 && co1 < CALIB_POS1_NUM && co2 >= 0 && co2 < CALIB_POS2_NUM) {
		co2--;
		if (co2 < 0) {
			co2 = CALIB_POS2_NUM - 1;
			co1--;
			if (co1 < 0) ret = true;
		}
	}

	*co1_p = co1;
	*co2_p = co2;
	return ret;
}

int calib(ARdouble *fovy_p, ARdouble *aspect_p, ARdouble m[16])
{
	return (calc_optical((ARdouble (*)[3])calib_pos3d, (ARdouble (*)[2])calib_pos2d, CALIB_POS1_NUM*CALIB_POS2_NUM,
                                 fovy_p, aspect_p, m));
}

// Save parameters file and index file with info about it, then signal thread that it's ready for upload.
void saveParam(const char *paramPathname, ARdouble fovy, ARdouble aspect, const ARdouble m[16])
{
	int i;
    if (arParamSaveOptical(paramPathname, fovy, aspect, m) < 0) {

        LOGE("Error writing optical parameters file '%s'.\n", paramPathname);

    } else {

	    LOGI("Wrote optical parameters to file '%s'.\n,", paramPathname);
    }
}


