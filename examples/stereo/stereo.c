/*
 *  stereo.c
 *  ARToolKit5
 *
 *  Some code to demonstrate use of ARToolKit running in with calibration
 *  files for a stereo optical see-through head mounted display (generated
 *  by calib_optical).
 *
 *  Press '?' while running for help on available key commands.
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
 *  Copyright 2007-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Philip Lamb.
 *
 */


// ============================================================================
//	Includes
// ============================================================================

#include <stdio.h>
#include <stdlib.h>					// malloc(), free()
#include <string.h>
#ifdef _WIN32
#  define snprintf _snprintf
#  define _USE_MATH_DEFINES
#endif
#include <math.h>
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include <AR/config.h>
#include <AR/video.h>
#include <AR/param.h>			// arParamDisp()
#include <AR/ar.h>
#include <AR/gsub_lite.h>
#include <ARUtil/time.h>
#include "ARMarkerSquare.h"
#include "VirtualEnvironment2.h"

// ============================================================================
//	Constants
// ============================================================================

#define VIEW_SCALEFACTOR		1.0         // Units received from ARToolKit tracking will be multiplied by this factor before being used in OpenGL drawing.
#define VIEW_DISTANCE_MIN		40.0        // Objects closer to the camera than this will not be displayed. OpenGL units.
#define VIEW_DISTANCE_MAX		10000.0     // Objects further away from the camera than this will not be displayed. OpenGL units.

#define FONT_SIZE 12.0f
#define FONT_LINE_SPACING 4.0f
#define FONT_SCALEFACTOR  ((FONT_SIZE)/119.05f) // Factor used to scale GLUT_STROKE_ROMAN font to a point size.

#undef STEREO_SUPPORT_MODES_REQUIRING_STENCIL // Not enabled as full-window stencilling is very slow on most hardware.

// ============================================================================
//	Global variables
// ============================================================================

// Preferences.
static int prefWindowed = TRUE;           // Use windowed (TRUE) or fullscreen mode (FALSE) on launch.
static int prefWidth = 1280;              // Preferred initial window width.
static int prefHeight = 480;              // Preferred initial window height.
static int prefDepth = 0;                 // Fullscreen mode bit depth. Set to 0 to use default depth.
static int prefRefresh = 0;				  // Fullscreen mode refresh rate. Set to 0 to use default rate.

// Image acquisition.
static AR2VideoParamT *gVidL = NULL;
static AR2VideoParamT *gVidR = NULL;
static AR2VideoBufferT *gARTImageL = NULL;
static AR2VideoBufferT *gARTImageR = NULL;

// Markers.
static ARMarkerSquare *markersSquare = NULL;
static int markersSquareCount = 0;

// Marker detection.
static ARHandle		*gARHandleL = NULL;
static ARHandle		*gARHandleR = NULL;
static long			 gCallCountMarkerDetect = 0;
static ARPattHandle	*gARPattHandle = NULL;
static int           gARPattDetectionMode;

// Transformation matrix retrieval.
static AR3DHandle	*gAR3DHandleL = NULL;
static AR3DHandle	*gAR3DHandleR = NULL;
static AR3DStereoHandle	*gAR3DStereoHandle = NULL;
static ARdouble      transL2R[3][4];
static ARdouble      transR2L[3][4];


// Drawing.
static ARParamLT *gCparamLTL = NULL;
static ARParamLT *gCparamLTR = NULL;
static int gVideoSeeThrough = 0;
static int gShowHelp = 1;
static int gShowMode = 0;

typedef enum {
	VIEW_LEFTEYE        = 1,
	VIEW_RIGHTEYE       = 2,
} VIEW_EYE_t;

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

static STEREO_DISPLAY_MODE stereoDisplayMode = STEREO_DISPLAY_MODE_SIDE_BY_SIDE;
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
    ARdouble cameraLens[16];
    ARdouble cameraPose[16];
    int cameraPoseValid;
    ARGL_CONTEXT_SETTINGS_REF arglSettings; // Per-view ARGL settings.
    VirtualEnvironment2 *ve2;
} VIEW_t;

static int viewCount;
static VIEW_t *views;

typedef struct {
    int contextIndex;       // An index number by which this context is referred to. For GLUT-managed contexts, this is the GLUT window index.
    int width;              // Width in pixels of this context. Note that this may not be the same as the width of any referred-to view(s).
    int height;             // Height in pixels of this context. Note that this may not be the same as the height of any referred-to view(s).
    int viewCount;          // Number of views referred to by this context.
    VIEW_t **views;         // Reference(s) to view(s). Not owned, and should not be de de-alloced when this struct is dealloced.
} VIEW_CONTEXT_t;

static int viewContextCount = 0;
static VIEW_CONTEXT_t *viewContexts;

// ============================================================================
//	Function prototypes.
// ============================================================================

static void usage(char *com);
static int setupCamera(const char *cparam_name, char *vconf, AR2VideoParamT **vid_p, ARParamLT **cparamLT_p);
static void cleanup(void);
static void Keyboard(unsigned char key, int x, int y);
static void Visibility(int visible);
static void Reshape(int w, int h);
static void Display(void);
static void DisplayPerContext(const int contextIndex);
static void print(const char *text, const float x, const float y, const int contentWidth, const int contentHeight, int calculateXFromRightEdge, int calculateYFromTopEdge);
static void drawBackground(const float width, const float height, const float x, const float y);
static void printHelpKeys(const int contentWidth, const int contentHeight);
static void printMode(const int contentWidth, const int contentHeight);

// ============================================================================
//	Functions
// ============================================================================

int main(int argc, char** argv)
{
	char    glutGamemode[32] = "";
    char   *vconfL = NULL;
    char   *vconfR = NULL;
    char   *cparam_nameL = NULL;
    char   *cparam_nameR = NULL;
    char    transL2RDefault[] = "Data/transL2R.dat";
    
    int     i, j;
    int     gotTwoPartOption;
    const char markerConfigDataFilename[] = "Data/markers.dat";
	const char objectDataFilename[] = "Data/objects.dat";
	
    //
	// Process command-line options.
	//
    
	glutInit(&argc, argv);
    
    arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST, NULL);
    
    i = 1; // argv[0] is name of app, so start at 1.
    while (i < argc) {
        gotTwoPartOption = FALSE;
        // Look for two-part options first.
        if ((i + 1) < argc) {
            if (strcmp(argv[i], "--vconfL") == 0) {
                i++;
                vconfL = argv[i];
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--vconfR") == 0) {
                i++;
                vconfR = argv[i];
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--cparaL") == 0) {
                i++;
                cparam_nameL = argv[i];
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--cparaR") == 0) {
                i++;
                cparam_nameR = argv[i];
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--stereo") == 0) {
                i++;
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
                else ARLOGe("Invalid stereo mode '%s' requested. Ignoring.\n", argv[i]);
#ifndef STEREO_SUPPORT_MODES_REQUIRING_STENCIL
                if (stereoDisplayMode == STEREO_DISPLAY_MODE_ROW_INTERLACED || stereoDisplayMode == STEREO_DISPLAY_MODE_COLUMN_INTERLACED || stereoDisplayMode == STEREO_DISPLAY_MODE_CHECKERBOARD) {
                    ARLOGe("Stereo mode '%s' not supported in this version. Using mono.\n", argv[i]);
                    stereoDisplayMode = STEREO_DISPLAY_MODE_INACTIVE;
                }
#endif
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i],"--width") == 0) {
                i++;
                // Get width from second field.
                if (sscanf(argv[i], "%d", &prefWidth) != 1) {
                    ARLOGe("Error: --width option must be followed by desired width.\n");
                }
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i],"--height") == 0) {
                i++;
                // Get height from second field.
                if (sscanf(argv[i], "%d", &prefHeight) != 1) {
                    ARLOGe("Error: --height option must be followed by desired height.\n");
                }
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i],"--refresh") == 0) {
                i++;
                // Get refresh rate from second field.
                if (sscanf(argv[i], "%d", &prefRefresh) != 1) {
                    ARLOGe("Error: --refresh option must be followed by desired refresh rate.\n");
                }
                gotTwoPartOption = TRUE;
            }
        }
        if (!gotTwoPartOption) {
            // Look for single-part options.
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
                usage(argv[0]);
            } else if( strncmp(argv[i], "-cparaL=", 8) == 0 ) {
                cparam_nameL = &(argv[i][8]);
            } else if( strncmp(argv[i], "-cparaR=", 8) == 0 ) {
                cparam_nameR = &(argv[i][8]);
            } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
                ARLOG("%s version %s\n", argv[0], AR_HEADER_VERSION_STRING);
                exit(0);
            } else if (strcmp(argv[i], "--blueline") == 0) {
                stereoDisplayUseBlueLine = TRUE;
            } else if (strcmp(argv[i],"--windowed") == 0) {
                prefWindowed = TRUE;
            } else if (strcmp(argv[i],"--fullscreen") == 0) {
                prefWindowed = FALSE;
            } else {
                ARLOGe("Error: invalid command line argument '%s'.\n", argv[i]);
                usage(argv[0]);
            }
        }
        i++;
    }
    

	//
	// Video setup.
	//
    
	if (!setupCamera(cparam_nameL, vconfL, &gVidL, &gCparamLTL)) {
		ARLOGe("main(): Unable to set up AR camera.\n");
		exit(-1);
	}
	if (!setupCamera(cparam_nameR, vconfR, &gVidR, &gCparamLTR)) {
		ARLOGe("main(): Unable to set up AR camera.\n");
		exit(-1);
	}
	
    //
    // AR init.
    //
    
    // Init AR.
    gARPattHandle = arPattCreateHandle();
	if (!gARPattHandle) {
		ARLOGe("Error creating pattern handle.\n");
		exit(-1);
	}
    
    gARHandleL = arCreateHandle(gCparamLTL);
    gARHandleR = arCreateHandle(gCparamLTR);
    if (!gARHandleL || !gARHandleR) {
        ARLOGe("Error creating AR handle.\n");
		exit(-1);
    }
    arPattAttach(gARHandleL, gARPattHandle);
    arPattAttach(gARHandleR, gARPattHandle);
    
    if (arSetPixelFormat(gARHandleL, ar2VideoGetPixelFormat(gVidL)) < 0 ||
        arSetPixelFormat(gARHandleR, ar2VideoGetPixelFormat(gVidR)) < 0) {
        ARLOGe("Error setting pixel format.\n");
		exit(-1);
    }
    
    gAR3DHandleL = ar3DCreateHandle(&gCparamLTL->param);
    gAR3DHandleR = ar3DCreateHandle(&gCparamLTR->param);
    if (!gAR3DHandleL || !gAR3DHandleR) {
        ARLOGe("Error creating 3D handle.\n");
		exit(-1);
    }
    
    if (arParamLoadExt(transL2RDefault, transL2R) < 0) {
        ARLOGe("Error: arParamLoadExt.\n");
        exit(-1);
    }
    arUtilMatInv((const ARdouble (*)[4])transL2R, transR2L);
    arParamDispExt(transL2R);
    gAR3DStereoHandle = ar3DStereoCreateHandle(&(gCparamLTL->param), &(gCparamLTR->param), AR_TRANS_MAT_IDENTITY, transL2R);
    if (!gAR3DStereoHandle) {
        ARLOGe("Error: ar3DCreateHandle.\n");
        exit(-1);
    }

    //
    // Markers setup.
    //
    
    // Load marker(s).
    newMarkers(markerConfigDataFilename, gARPattHandle, &markersSquare, &markersSquareCount, &gARPattDetectionMode);
    ARLOGi("Marker count = %d\n", markersSquareCount);
    
    //
    // Other ARToolKit setup.
    //
    
    arSetMarkerExtractionMode(gARHandleL, AR_USE_TRACKING_HISTORY_V2);
    arSetMarkerExtractionMode(gARHandleR, AR_USE_TRACKING_HISTORY_V2);
    //arSetMarkerExtractionMode(gARHandleL, AR_NOUSE_TRACKING_HISTORY);
    //arSetMarkerExtractionMode(gARHandleR, AR_NOUSE_TRACKING_HISTORY);
    //arSetLabelingThreshMode(gARHandleL, AR_LABELING_THRESH_MODE_MANUAL); // Uncomment to force manual thresholding.
    //arSetLabelingThreshMode(gARHandleR, AR_LABELING_THRESH_MODE_MANUAL); // Uncomment to force manual thresholding.
    
    // Set the pattern detection mode (template (pictorial) vs. matrix (barcode) based on
    // the marker types as defined in the marker config. file.
    arSetPatternDetectionMode(gARHandleL, gARPattDetectionMode); // Default = AR_TEMPLATE_MATCHING_COLOR
    arSetPatternDetectionMode(gARHandleR, gARPattDetectionMode); // Default = AR_TEMPLATE_MATCHING_COLOR
    
    // Other application-wide marker options. Once set, these apply to all markers in use in the application.
    // If you are using standard ARToolKit picture (template) markers, leave commented to use the defaults.
    // If you are usign a different marker design (see http://www.artoolworks.com/support/app/marker.php )
    // then uncomment and edit as instructed by the marker design application.
    //arSetLabelingMode(gARHandleL, AR_LABELING_BLACK_REGION); // Default = AR_LABELING_BLACK_REGION
    //arSetLabelingMode(gARHandleR, AR_LABELING_BLACK_REGION); // Default = AR_LABELING_BLACK_REGION
    //arSetBorderSize(gARHandleL, 0.25f); // Default = 0.25f
    //arSetBorderSize(gARHandleR, 0.25f); // Default = 0.25f
    //arSetMatrixCodeType(gARHandleL, AR_MATRIX_CODE_3x3); // Default = AR_MATRIX_CODE_3x3
    //arSetMatrixCodeType(gARHandleR, AR_MATRIX_CODE_3x3); // Default = AR_MATRIX_CODE_3x3
    
	//
	// Graphics setup.
	//
    
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
            viewContexts[0].views[0]->drawBuffer = (stereoDisplayMode == STEREO_DISPLAY_MODE_QUADBUFFERED ? GL_BACK_LEFT : GL_BACK);
            viewContexts[0].views[1] = &(views[1]);
            viewContexts[0].views[1]->viewEye = (!stereoDisplayReverseLeftRight ? VIEW_RIGHTEYE : VIEW_LEFTEYE); // Right second, unless reversed.
            viewContexts[0].views[1]->drawBuffer = (stereoDisplayMode == STEREO_DISPLAY_MODE_QUADBUFFERED ? GL_BACK_RIGHT : GL_BACK);
        }
    }
    
    // Now create the contexts.
    if (stereoDisplayMode == STEREO_DISPLAY_MODE_QUADBUFFERED) {
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STEREO);
        ARLOGi("Using GLUT quad-buffered stereo window mode.\n");
#ifdef STEREO_SUPPORT_MODES_REQUIRING_STENCIL
    } else if (stereoDisplayMode == STEREO_DISPLAY_MODE_ROW_INTERLACED || stereoDisplayMode == STEREO_DISPLAY_MODE_COLUMN_INTERLACED || stereoDisplayMode == STEREO_DISPLAY_MODE_CHECKERBOARD) {
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
#endif
    } else {
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    }
    if (prefWindowed) {
        if (prefWidth > 0 && prefHeight > 0) glutInitWindowSize(prefWidth, prefHeight);
        else glutInitWindowSize(gCparamLTL->param.xsize, gCparamLTL->param.ysize);
        for (i = 0; i < viewContextCount; i++) {
            viewContexts[i].contextIndex = glutCreateWindow(argv[0]);
        }
    } else {
        if (glutGameModeGet(GLUT_GAME_MODE_POSSIBLE)) {
            if (prefWidth && prefHeight) {
                if (prefDepth) {
                    if (prefRefresh) snprintf(glutGamemode, sizeof(glutGamemode), "%ix%i:%i@%i", prefWidth, prefHeight, prefDepth, prefRefresh);
                    else snprintf(glutGamemode, sizeof(glutGamemode), "%ix%i:%i", prefWidth, prefHeight, prefDepth);
                } else {
                    if (prefRefresh) snprintf(glutGamemode, sizeof(glutGamemode), "%ix%i@%i", prefWidth, prefHeight, prefRefresh);
                    else snprintf(glutGamemode, sizeof(glutGamemode), "%ix%i", prefWidth, prefHeight);
                }
            } else {
                prefWidth = glutGameModeGet(GLUT_GAME_MODE_WIDTH);
                prefHeight = glutGameModeGet(GLUT_GAME_MODE_HEIGHT);
                snprintf(glutGamemode, sizeof(glutGamemode), "%ix%i", prefWidth, prefHeight);
            }
            glutGameModeString(glutGamemode);
            glutEnterGameMode();
            viewContexts[0].contextIndex = 0;
        } else {
            if (prefWidth > 0 && prefHeight > 0) glutInitWindowSize(prefWidth, prefHeight);
            viewContexts[0].contextIndex = glutCreateWindow(argv[0]);
            glutFullScreen();
        }
    }
    
	// Per- GL context setup (e.g. display lists, textures.)
    
    for (i = 0; i < viewContextCount; i++) {
        if (viewContexts[i].contextIndex) glutSetWindow(viewContexts[i].contextIndex);
        
        // Per-view setup.
        for (j = 0; j < viewContexts[i].viewCount; j++) {
            
            // Create the OpenGL projection from the calibrated camera parameters.
            arglCameraFrustumRH(&((viewContexts[i].views[j]->viewEye == VIEW_RIGHTEYE ? gCparamLTR : gCparamLTL)->param), VIEW_DISTANCE_MIN, VIEW_DISTANCE_MAX, viewContexts[i].views[j]->cameraLens);
            viewContexts[i].views[j]->cameraPoseValid = FALSE;
            
            // Setup ARgsub_lite library for current OpenGL view.
            if ((viewContexts[i].views[j]->arglSettings = arglSetupForCurrentContext(&((viewContexts[i].views[j]->viewEye == VIEW_RIGHTEYE ? gCparamLTR : gCparamLTL)->param), ar2VideoGetPixelFormat((viewContexts[i].views[j]->viewEye == VIEW_RIGHTEYE ? gVidR : gVidL)))) == NULL) {
                ARLOGe("main(): arglSetupForCurrentContext() returned error.\n");
                cleanup();
                exit(-1);
            }
            arglSetupDebugMode(viewContexts[i].views[j]->arglSettings, (viewContexts[i].views[j]->viewEye == VIEW_RIGHTEYE ? gARHandleR : gARHandleL));
            
            // Load objects (i.e. OSG models).
            viewContexts[i].views[j]->ve2 = VirtualEnvironment2Init(objectDataFilename);
            VirtualEnvironment2HandleARViewUpdatedCameraLens(viewContexts[i].views[j]->ve2, viewContexts[i].views[j]->cameraLens);
            
        }
        
    }
	
    //
    // Setup complete. Start tracking.
    //
    
    // Start the video.
    if (ar2VideoCapStart(gVidL) != 0 ||
        ar2VideoCapStart(gVidR) != 0) {
    	ARLOGe("setupCamera(): Unable to begin camera data capture.\n");
		return (FALSE);
	}
    arUtilTimerReset();
    
	// Register GLUT event-handling callbacks.
	// NB: mainLoop() is registered by Visibility.
    for (i = 0; i < viewContextCount; i++) {
        if (viewContexts[i].contextIndex) glutSetWindow(viewContexts[i].contextIndex);
        glutDisplayFunc(Display);
        glutReshapeFunc(Reshape);
        glutVisibilityFunc(Visibility);
    }
	glutKeyboardFunc(Keyboard);
	
	glutMainLoop();
    
	return (0);
}

static void usage(char *com)
{
    ARLOG("Usage: %s [options]\n", com);
    ARLOG("Options:\n");
    ARLOG("  --cparaL <camera parameter file for the Left camera>\n");
    ARLOG("  --cparaR <camera parameter file for the Right camera>\n");
    ARLOG("  -cparaL=<camera parameter file for the Left camera>\n");
    ARLOG("  -cparaR=<camera parameter file for the Right camera>\n");
    ARLOG("  --vconfL <video parameter for the Left camera>\n");
    ARLOG("  --vconfR <video parameter for the Right camera>\n");
	ARLOG("  --width w     Use display/window width of w pixels.\n");
	ARLOG("  --height h    Use display/window height of h pixels.\n");
	ARLOG("  --refresh f   Use display refresh rate of f Hz.\n");
	ARLOG("  --windowed    Display in window, rather than fullscreen.\n");
	ARLOG("  --fullscreen  Display fullscreen, rather than in window.\n");
    ARLOG("  --stereo [INACTIVE|DUAL_OUTPUT|QUADBUFFERED|FRAME_SEQUENTIAL|\n");
    ARLOG("            SIDE_BY_SIDE|OVER_UNDER|HALF_SIDE_BY_SIDE|\n");
    ARLOG("            OVER_UNDER_HALF_HEIGHT|ANAGLYPH_RED_BLUE|ANAGLYPH_RED_GREEN\n");
    ARLOG("            ROW_INTERLACED|COLUMN_INTERLACED|CHECKERBOARD].\n");
    ARLOG("            Select mono or stereo mode. (Not all modes supported).\n");
    ARLOG("  -h -help --help: show this message\n");
    exit(0);
}

static int setupCamera(const char *cparam_name, char *vconf, AR2VideoParamT **vid_p, ARParamLT **cparamLT_p)
{	
    ARParam			cparam;
	int				xsize, ysize;
    AR_PIXEL_FORMAT pixFormat;

    // Open the video path.
    if (!(*vid_p = ar2VideoOpen(vconf))) {
    	ARLOGe("setupCamera(): Unable to open connection to camera.\n");
    	return (FALSE);
	}
	
    // Find the size of the window.
    if (ar2VideoGetSize(*vid_p, &xsize, &ysize) < 0) {
        ARLOGe("setupCamera(): Unable to determine camera frame size.\n");
        ar2VideoClose(*vid_p);
        return (FALSE);
    }
    ARLOGi("Camera image size (x,y) = (%d,%d)\n", xsize, ysize);
	
	// Get the format in which the camera is returning pixels.
	pixFormat = ar2VideoGetPixelFormat(*vid_p);
	if (pixFormat == AR_PIXEL_FORMAT_INVALID) {
    	ARLOGe("setupCamera(): Camera is using unsupported pixel format.\n");
        ar2VideoClose(*vid_p);
		return (FALSE);
	}
	
	// Load the camera parameters, resize for the window and init.
	if (cparam_name && *cparam_name) {
        if (arParamLoad(cparam_name, 1, &cparam) < 0) {
		    ARLOGe("setupCamera(): Error loading parameter file %s for camera.\n", cparam_name);
            arVideoClose();
            return (FALSE);
        }
    } else {
        arParamClearWithFOVy(&cparam, xsize, ysize, M_PI_4); // M_PI_4 radians = 45 degrees.
        ARLOGw("Using default camera parameters for %dx%d image size, 45 degrees vertical field-of-view.\n", xsize, ysize);
    }
    if (cparam.xsize != xsize || cparam.ysize != ysize) {
        ARLOGw("*** Camera Parameter resized from %d, %d. ***\n", cparam.xsize, cparam.ysize);
        arParamChangeSize(&cparam, xsize, ysize, &cparam);
    }
#ifdef DEBUG
    ARLOG("*** Camera Parameter ***\n");
    arParamDisp(&cparam);
#endif
    if ((*cparamLT_p = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
        ARLOGe("setupCamera(): Error: arParamLTCreate.\n");
        ar2VideoClose(*vid_p);
        return (FALSE);
    }

	return (TRUE);
}

static void cleanup(void)
{
    int i, j;
    
#ifdef STEREO_SUPPORT_MODES_REQUIRING_STENCIL
    if (stereoStencilPattern) free(stereoStencilPattern);
#endif
    
    if (viewContexts) {
        for (i = 0; i < viewContextCount; i++) {
            
            for (j = 0; j < viewContexts[i].viewCount; j++) {
                VirtualEnvironment2Final(&viewContexts[i].views[j]->ve2);
                if (viewContexts[i].views[j]->arglSettings) arglCleanup(viewContexts[i].views[j]->arglSettings);
            }
            
            free(viewContexts[i].views);
        }
        free(viewContexts);
        viewContextCount = 0;
    }
    if (views) {
        free(views);
        viewCount = 0;
    }
    
	arPattDetach(gARHandleL);
	arPattDetach(gARHandleR);
	arPattDeleteHandle(gARPattHandle);
	ar2VideoCapStop(gVidL);
	ar2VideoCapStop(gVidR);
	ar3DStereoDeleteHandle(&gAR3DStereoHandle);
	ar3DDeleteHandle(&gAR3DHandleL);
	ar3DDeleteHandle(&gAR3DHandleR);
	arDeleteHandle(gARHandleL);
	arDeleteHandle(gARHandleR);
    arParamLTFree(&gCparamLTL);
    arParamLTFree(&gCparamLTR);
	ar2VideoClose(gVidL); gVidL = NULL;
	ar2VideoClose(gVidR); gVidR = NULL;
}

static void Keyboard(unsigned char key, int x, int y)
{
	int mode, threshChange = 0;
    AR_LABELING_THRESH_MODE modea;
	
	switch (key) {
		case 0x1B:						// Quit.
		case 'Q':
		case 'q':
			cleanup();
			exit(0);
			break;
		case 'X':
		case 'x':
            arGetImageProcMode(gARHandleL, &mode);
            switch (mode) {
                case AR_IMAGE_PROC_FRAME_IMAGE:  mode = AR_IMAGE_PROC_FIELD_IMAGE; break;
                case AR_IMAGE_PROC_FIELD_IMAGE:
                default: mode = AR_IMAGE_PROC_FRAME_IMAGE; break;
            }
            arSetImageProcMode(gARHandleL, mode);
            arSetImageProcMode(gARHandleR, mode);
			break;
		case 'C':
		case 'c':
			ARLOGe("*** Camera - %f (frame/sec)\n", (double)gCallCountMarkerDetect/arUtilTimer());
			gCallCountMarkerDetect = 0;
			arUtilTimerReset();
			break;
		case 'a':
		case 'A':
			arGetLabelingThreshMode(gARHandleL, &modea);
            switch (modea) {
                case AR_LABELING_THRESH_MODE_MANUAL:        modea = AR_LABELING_THRESH_MODE_AUTO_MEDIAN; break;
                case AR_LABELING_THRESH_MODE_AUTO_MEDIAN:   modea = AR_LABELING_THRESH_MODE_AUTO_OTSU; break;
                case AR_LABELING_THRESH_MODE_AUTO_OTSU:     modea = AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE; break;
                case AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE: modea = AR_LABELING_THRESH_MODE_AUTO_BRACKETING; break;
                case AR_LABELING_THRESH_MODE_AUTO_BRACKETING:
                default: modea = AR_LABELING_THRESH_MODE_MANUAL; break;
            }
            arSetLabelingThreshMode(gARHandleL, modea);
            arSetLabelingThreshMode(gARHandleR, modea);
			break;
		case '-':
			threshChange = -5;
			break;
		case '+':
		case '=':
			threshChange = +5;
			break;
		case 'D':
		case 'd':
			arGetDebugMode(gARHandleL, &mode);
			arSetDebugMode(gARHandleL, !mode);
			arSetDebugMode(gARHandleR, !mode);
			break;
		case '?':
		case '/':
            gShowHelp++;
            if (gShowHelp > 1) gShowHelp = 0;
			break;
        case 'm':
        case 'M':
            gShowMode = !gShowMode;
            break;
		case 'O':
		case 'o':
			gVideoSeeThrough = !gVideoSeeThrough;
			break;
		default:
			break;
	}
	if (threshChange) {
		int threshhold;
		arGetLabelingThresh(gARHandleL, &threshhold);
		threshhold += threshChange;
		if (threshhold < 0) threshhold = 0;
		if (threshhold > 255) threshhold = 255;
		arSetLabelingThresh(gARHandleL, threshhold);
		arSetLabelingThresh(gARHandleR, threshhold);
	}
	
}

static void mainLoop(void)
{
	static int ms_prev;
	int ms;
	float s_elapsed;
    AR2VideoBufferT *buffL, *buffR;
    ARMarkerInfo* markerInfoL;
    ARMarkerInfo* markerInfoR;
    int markerNumL;
    int markerNumR;
	ARdouble err;
    ARdouble transR[3][4];
    ARPose poseR;
    int             i, j, kL, kR;
	
	// Calculate time delta.
	ms = glutGet(GLUT_ELAPSED_TIME);
	s_elapsed = (float)(ms - ms_prev) * 0.001f;
	ms_prev = ms;
	
	// Grab a video frame.
    buffL = ar2VideoGetImage(gVidL);
    buffR = ar2VideoGetImage(gVidR);
    if (buffL && buffL->fillFlag) {
        gARTImageL = buffL;
    }
	if (buffR && buffR->fillFlag) {
        gARTImageR = buffR;
    }
    
    if (gARTImageL && gARTImageR) {
		
        for (i = 0; i < viewCount; i++) {
            if (views[i].viewEye == VIEW_RIGHTEYE) {
                arglPixelBufferDataUpload(views[i].arglSettings, gARTImageR->buff);
            } else {
                arglPixelBufferDataUpload(views[i].arglSettings, gARTImageL->buff);
            }
        }

		gCallCountMarkerDetect++; // Increment ARToolKit FPS counter.
		
		// Detect the markers in the video frame.
		if (arDetectMarker(gARHandleL, gARTImageL) < 0 ||
            arDetectMarker(gARHandleR, gARTImageR) < 0) {
			exit(-1);
		}
		
		// Get detected markers
		markerInfoL = arGetMarker(gARHandleL);
		markerInfoR = arGetMarker(gARHandleR);
		markerNumL = arGetMarkerNum(gARHandleL);
		markerNumR = arGetMarkerNum(gARHandleR);
        
		// Update markers.
		for (i = 0; i < markersSquareCount; i++) {
			markersSquare[i].validPrev = markersSquare[i].valid;
            markersSquare[i].valid = FALSE;
            
			// Check through the marker_info array for highest confidence
			// visible marker matching our preferred pattern.
			kL = kR = -1;
			if (markersSquare[i].patt_type == AR_PATTERN_TYPE_TEMPLATE) {
				for (j = 0; j < markerNumL; j++) {
					if (markersSquare[i].patt_id == markerInfoL[j].idPatt) {
						if (kL == -1) {
							if (markerInfoL[j].cfPatt >= markersSquare[i].matchingThreshold) kL = j; // First marker detected.
						} else if (markerInfoL[j].cfPatt > markerInfoL[kL].cfPatt) kL = j; // Higher confidence marker detected.
					}
				}
				if (kL != -1) {
					markerInfoL[kL].id = markerInfoL[kL].idPatt;
					markerInfoL[kL].cf = markerInfoL[kL].cfPatt;
					markerInfoL[kL].dir = markerInfoL[kL].dirPatt;
				}
				for (j = 0; j < markerNumR; j++) {
					if (markersSquare[i].patt_id == markerInfoR[j].idPatt) {
						if (kR == -1) {
							if (markerInfoR[j].cfPatt >= markersSquare[i].matchingThreshold) kR = j; // First marker detected.
						} else if (markerInfoR[j].cfPatt > markerInfoR[kR].cfPatt) kR = j; // Higher confidence marker detected.
					}
				}
				if (kR != -1) {
					markerInfoR[kR].id = markerInfoR[kR].idPatt;
					markerInfoR[kR].cf = markerInfoR[kR].cfPatt;
					markerInfoR[kR].dir = markerInfoR[kR].dirPatt;
				}
			} else {
				for (j = 0; j < markerNumL; j++) {
					if (markersSquare[i].patt_id == markerInfoL[j].idMatrix) {
						if (kL == -1) {
							if (markerInfoL[j].cfMatrix >= markersSquare[i].matchingThreshold) kL = j; // First marker detected.
						} else if (markerInfoL[j].cfMatrix > markerInfoL[kL].cfMatrix) kL = j; // Higher confidence marker detected.
					}
				}
				if (kL != -1) {
					markerInfoL[kL].id = markerInfoL[kL].idMatrix;
					markerInfoL[kL].cf = markerInfoL[kL].cfMatrix;
					markerInfoL[kL].dir = markerInfoL[kL].dirMatrix;
				}
				for (j = 0; j < markerNumR; j++) {
					if (markersSquare[i].patt_id == markerInfoR[j].idMatrix) {
						if (kR == -1) {
							if (markerInfoR[j].cfMatrix >= markersSquare[i].matchingThreshold) kR = j; // First marker detected.
						} else if (markerInfoR[j].cfMatrix > markerInfoR[kR].cfMatrix) kR = j; // Higher confidence marker detected.
					}
				}
				if (kR != -1) {
					markerInfoR[kR].id = markerInfoR[kR].idMatrix;
					markerInfoR[kR].cf = markerInfoR[kR].cfMatrix;
					markerInfoR[kR].dir = markerInfoR[kR].dirMatrix;
				}
			}
            
			if (kL != -1 || kR != -1) {
                
                if (kL != -1 && kR != -1) {
                    err = arGetStereoMatchingErrorSquare(gAR3DStereoHandle, &markerInfoL[kL], &markerInfoR[kR]);
                    //ARLOG("stereo err = %f\n", err);
                    if (err > 16.0) {
                        //ARLOG("Stereo matching error: %d %d.\n", markerInfoL[kL].area, markerInfoR[kR].area);
                        if (markerInfoL[kL].area > markerInfoR[kR].area ) kR = -1;
                        else                                              kL = -1;
                    }
                }

                err = arGetTransMatSquareStereo(gAR3DStereoHandle, (kL == -1 ? NULL : &markerInfoL[kL]), (kR == -1 ?  NULL : &markerInfoR[kR]), markersSquare[i].marker_width, markersSquare[i].trans);
                
                if (err < 10.0) markersSquare[i].valid = TRUE;

                if (kL == -1)      ARLOG("[%2d] right:      err = %f\n", i, err);
                else if (kR == -1) ARLOG("[%2d] left:       err = %f\n", i, err);
                else               ARLOG("[%2d] left+right: err = %f\n", i, err);
                
			}
            
			if (markersSquare[i].valid) {
                
				// Filter the pose estimate.
				if (markersSquare[i].ftmi) {
					if (arFilterTransMat(markersSquare[i].ftmi, markersSquare[i].trans, !markersSquare[i].validPrev) < 0) {
						ARLOGe("arFilterTransMat error with marker %d.\n", i);
					}
				}
                
				if (!markersSquare[i].validPrev) {
					// Marker has become visible, tell any dependent objects.
                    for (j = 0; j < viewCount; j++) {
                        VirtualEnvironment2HandleARMarkerAppeared(views[j].ve2, i);
                    }
				}
                
				// We have a new pose, so set that.
				arglCameraViewRH((const ARdouble (*)[4])markersSquare[i].trans, markersSquare[i].pose.T, 1.0f /*VIEW_SCALEFACTOR*/);
                arUtilMatMul((const ARdouble (*)[4])transL2R, (const ARdouble (*)[4])markersSquare[i].trans, transR);
				arglCameraViewRH((const ARdouble (*)[4])transR, poseR.T, 1.0f /*VIEW_SCALEFACTOR*/);
				// Tell any dependent objects about the update.
                for (j = 0; j < viewCount; j++) {
                    VirtualEnvironment2HandleARMarkerWasUpdated(views[j].ve2, i, (views[j].viewEye == VIEW_RIGHTEYE ? poseR : markersSquare[i].pose));
                }
                
			} else {
                
				if (markersSquare[i].validPrev) {
					// Marker has ceased to be visible, tell any dependent objects.
                    for (j = 0; j < viewCount; j++) {
                        VirtualEnvironment2HandleARMarkerDisappeared(views[j].ve2, i);
                    }
				}
			}
        }
        
        // Invalidate the pointers so that next time we try to get new frames.
        gARTImageL = gARTImageR = NULL;

		// Tell GLUT the display has changed.
		glutPostRedisplay();
	} else {
		arUtilSleep(2);
	}
    

}

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

//
//	This function is called on events when the visibility of the
//	GLUT window changes (including when it first becomes visible).
//
static void Visibility(int visible)
{
	if (visible == GLUT_VISIBLE) {
		glutIdleFunc(mainLoop);
	} else {
		glutIdleFunc(NULL);
	}
}

//
//	This function is called when the
//	GLUT window is resized.
//
static void Reshape(int w, int h)
{
    int contextIndex;
    int viewPortWidth, viewPortHeight;
    float contentWidth, contentHeight;
    int viewIndex;
    //int r, c;
    
    contextIndex = findContextIndex();
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
    for (viewIndex = 0; viewIndex < (stereoDisplayMode == STEREO_DISPLAY_MODE_INACTIVE ? 1 : 2); viewIndex++) {
        VirtualEnvironment2HandleARViewUpdatedViewport(views[viewIndex].ve2, views[viewIndex].viewPort);
    }
    
	ARLOGe("Window was resized to %dx%d.\n", w, h);
}

//
// This function is called when the window needs redrawing.
//
static void Display(void)
{
	int contextIndex;
    
    contextIndex = findContextIndex();
    if (contextIndex >= 0 && contextIndex < viewContextCount) {
		DisplayPerContext(contextIndex);
		glutSwapBuffers();
    }
}

static void DisplayPerContext(const int contextIndex)
{
    int i;
    VIEW_t *view;
    VIEW_EYE_t viewEye;
    
    ARLOGe("DisplayPerContext(%d)\n", contextIndex);

    // Clear the buffer(s) now.
    glDrawBuffer(GL_BACK); // Includes both GL_BACK_LEFT and GL_BACK_RIGHT (if defined).
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
		glDrawBuffer(view->drawBuffer);
        
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
	
        if (viewEye == VIEW_RIGHTEYE) {
            arglPixelBufferDataUpload(view->arglSettings, gARTImageR->buff);
            arglDispImage(view->arglSettings);
            gARTImageR = NULL; // Invalidate image data.
        } else {
            arglPixelBufferDataUpload(view->arglSettings, gARTImageL->buff);
            arglDispImage(view->arglSettings);
            gARTImageL = NULL; // Invalidate image data.
        }

        // Set up 3D mode.
        glMatrixMode(GL_PROJECTION);
#ifdef ARDOUBLE_IS_FLOAT
        glLoadMatrixf(view->cameraLens);
#else
        glLoadMatrixd(view->cameraLens);
#endif
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glEnable(GL_DEPTH_TEST);
        
        // Set any initial per-frame GL state you require here.
        // --->
        
        // Lighting and geometry that moves with the camera should be added here.
        // (I.e. should be specified before camera pose transform.)
        // --->
        
        VirtualEnvironment2HandleARViewDrawPreCamera(view->ve2);
        
        if (view->cameraPoseValid) {
            
#ifdef ARDOUBLE_IS_FLOAT
            glMultMatrixf(view->cameraPose);
#else
            glMultMatrixd(view->cameraPose);
#endif
            
            // All lighting and geometry to be drawn in world coordinates goes here.
            // --->
            VirtualEnvironment2HandleARViewDrawPostCamera(view->ve2);
        }
        
        // Set up 2D mode.
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
		glOrtho(0.0, view->contentWidth, 0.0, view->contentHeight, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        
        // Add your own 2D overlays here.
        // --->
        
        VirtualEnvironment2HandleARViewDrawOverlay(view->ve2);
        
		//
		// Draw help text and mode.
		//
        glLoadIdentity();
        glDisable(GL_TEXTURE_2D);
        if (gShowMode) {
            printMode(view->contentWidth, view->contentHeight);
        }
        if (gShowHelp) {
            if (gShowHelp == 1) {
                printHelpKeys(view->contentWidth, view->contentHeight);
            }
        }

        if (stereoDisplayUseBlueLine) {
            glViewport(view->viewPort[viewPortIndexLeft], view->viewPort[viewPortIndexBottom], view->viewPort[viewPortIndexWidth], view->viewPort[viewPortIndexHeight]);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, view->viewPort[viewPortIndexWidth], 0, view->viewPort[viewPortIndexHeight], -1.0, 1.0);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            glLineWidth(1.0f);
            glColor4ub(0, 0, 0, 255);
            glBegin(GL_LINES); // Draw a background line
            glVertex3f(0.0f, 0.5f, 0.0f);
            glVertex3f(view->viewPort[viewPortIndexWidth], 0.5f, 0.0f);
            glEnd();
            glColor4ub(0, 0, 255, 255);
            glBegin(GL_LINES); // Draw a line of the correct length (the cross over is about 40% across the screen from the left
            glVertex3f(0.0f, 0.0f, 0.0f);
            if (viewEye == VIEW_LEFTEYE) {
                glVertex3f(view->viewPort[viewPortIndexWidth] * 0.30f, 0.5f, 0.0f);
            } else {
                glVertex3f(view->viewPort[viewPortIndexWidth] * 0.80f, 0.5f, 0.0f);
            }
            glEnd();
        }

        if (stereoDisplayMode == STEREO_DISPLAY_MODE_FRAME_SEQUENTIAL) {
            stereoDisplaySequentialNext = (stereoDisplaySequentialNext == VIEW_LEFTEYE ? VIEW_RIGHTEYE : VIEW_LEFTEYE);
        }
        
    } // end of view.

}

//
// The following functions provide the onscreen help text and mode info.
//

static void print(const char *text, const float x, const float y, const int contentWidth, const int contentHeight, int calculateXFromRightEdge, int calculateYFromTopEdge)
{
    int i, len;
    GLfloat x0, y0;
    
    if (!text) return;
    
    if (calculateXFromRightEdge) {
        x0 = contentWidth - x - (float)glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char *)text)*FONT_SCALEFACTOR;
    } else {
        x0 = x;
    }
    if (calculateYFromTopEdge) {
        y0 = contentHeight - y - FONT_SIZE;
    } else {
        y0 = y;
    }
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(x0, y0, 0.0f);
    glScalef(FONT_SCALEFACTOR, FONT_SCALEFACTOR, FONT_SCALEFACTOR);
    
    len = (int)strlen(text);
    for (i = 0; i < len; i++) glutStrokeCharacter(GLUT_STROKE_ROMAN, text[i]);
    
    glPopMatrix();
}

static void drawBackground(const float width, const float height, const float x, const float y)
{
    GLfloat vertices[4][2];
    
    vertices[0][0] = x; vertices[0][1] = y;
    vertices[1][0] = width + x; vertices[1][1] = y;
    vertices[2][0] = width + x; vertices[2][1] = height + y;
    vertices[3][0] = x; vertices[3][1] = height + y;
    
    glLoadIdentity();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);	// 50% transparent black.
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Opaque white.
    //glLineWidth(1.0f);
    //glDrawArrays(GL_LINE_LOOP, 0, 4);
    glPopClientAttrib();
    glDisable(GL_BLEND);
}

static void printHelpKeys(const int contentWidth, const int contentHeight)
{
    int i;
    GLfloat  w, bw, bh;
    const char *helpText[] = {
        "Keys:\n",
        " ? or /        Show/hide this help.",
        " q or [esc]    Quit program.",
        " o             Toggle optical / video see-through display.",
        " d             Activate / deactivate debug mode.",
        " m             Toggle display of mode info.",
        " a             Toggle between available threshold modes.",
        " - and +       Switch to manual threshold mode, and adjust threshhold up/down by 5.",
        " x             Change image processing mode.",
        " c             Calulcate frame rate.",
    };
#define helpTextLineCount (sizeof(helpText)/sizeof(char *))
	float hMargin = 2.0f;
	float vMargin = 2.0f;
    
    bw = 0.0f;
    for (i = 0; i < helpTextLineCount; i++) {
        w = (float)glutStrokeLength(GLUT_STROKE_ROMAN, (unsigned char *)helpText[i])*FONT_SCALEFACTOR;
        if (w > bw) bw = w;
    }
    bh = helpTextLineCount * FONT_SIZE + (helpTextLineCount - 1) * FONT_LINE_SPACING;
    drawBackground(bw, bh, hMargin, vMargin);
    
    for (i = 0; i < helpTextLineCount; i++) print(helpText[i], hMargin, vMargin + (helpTextLineCount - 1 - i)*(FONT_SIZE + FONT_LINE_SPACING), contentWidth, contentHeight, 0, 0);
}

static void printMode(const int contentWidth, const int contentHeight)
{
    int len, thresh, line, mode, xsize, ysize;
    AR_LABELING_THRESH_MODE threshMode;
    ARdouble tempF;
    char text[256], *text_p;
	float hMargin = 2.0f;
	float vMargin = 2.0f;
    
    glColor4ub(255, 255, 255, 255);
    line = 1;
    
    // Image size and processing mode.
    ar2VideoGetSize(gVidL, &xsize, &ysize);
    arGetImageProcMode(gARHandleL, &mode);
	if (mode == AR_IMAGE_PROC_FRAME_IMAGE) text_p = "full frame";
	else text_p = "even field only";
    snprintf(text, sizeof(text), "Processing %dx%d video frames %s", xsize, ysize, text_p);
    print(text, hMargin, (line - 1)*(FONT_SIZE + FONT_LINE_SPACING) + vMargin, contentWidth, contentHeight, 0, 1);
    line++;
    
    // Threshold mode, and threshold, if applicable.
    arGetLabelingThreshMode(gARHandleL, &threshMode);
    switch (threshMode) {
        case AR_LABELING_THRESH_MODE_MANUAL: text_p = "MANUAL"; break;
        case AR_LABELING_THRESH_MODE_AUTO_MEDIAN: text_p = "AUTO_MEDIAN"; break;
        case AR_LABELING_THRESH_MODE_AUTO_OTSU: text_p = "AUTO_OTSU"; break;
        case AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE: text_p = "AUTO_ADAPTIVE"; break;
        case AR_LABELING_THRESH_MODE_AUTO_BRACKETING: text_p = "AUTO_BRACKETING"; break;
        default: text_p = "UNKNOWN"; break;
    }
    snprintf(text, sizeof(text), "Threshold mode: %s", text_p);
    if (threshMode != AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE) {
        arGetLabelingThresh(gARHandleL, &thresh);
        len = (int)strlen(text);
        snprintf(text + len, sizeof(text) - len, ", thresh=%d", thresh);
    }
    print(text, hMargin, (line - 1)*(FONT_SIZE + FONT_LINE_SPACING) + vMargin, contentWidth, contentHeight, 0, 1);
    line++;
    
    // Border size, image processing mode, pattern detection mode.
    arGetBorderSize(gARHandleL, &tempF);
    snprintf(text, sizeof(text), "Border: %0.1f%%", tempF*100.0);
    arGetPatternDetectionMode(gARHandleL, &mode);
    switch (mode) {
        case AR_TEMPLATE_MATCHING_COLOR: text_p = "Colour template (pattern)"; break;
        case AR_TEMPLATE_MATCHING_MONO: text_p = "Mono template (pattern)"; break;
        case AR_MATRIX_CODE_DETECTION: text_p = "Matrix (barcode)"; break;
        case AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX: text_p = "Colour template + Matrix (2 pass, pattern + barcode)"; break;
        case AR_TEMPLATE_MATCHING_MONO_AND_MATRIX: text_p = "Mono template + Matrix (2 pass, pattern + barcode "; break;
        default: text_p = "UNKNOWN"; break;
    }
    len = (int)strlen(text);
    snprintf(text + len, sizeof(text) - len, ", Pattern detection mode: %s", text_p);
    print(text, hMargin,  (line - 1)*(FONT_SIZE + FONT_LINE_SPACING) + vMargin, contentWidth, contentHeight, 0, 1);
    line++;    
}
