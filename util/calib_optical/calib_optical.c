/*
 *  calib_optical.c
 *  ARToolKit5
 *
 *  Camera calibration utility.
 *
 *  Press '?' while running for help on available key commands. *
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
 *  Copyright 2007-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */


// ============================================================================
//	Includes
// ============================================================================

#include <stdio.h>
#include <stdlib.h>					// malloc(), free()
#include <string.h>
#ifndef _MSC_VER
#  include <stdbool.h>
#else
typedef unsigned char bool;
#  define false 0
#  define true 1
#endif
#ifdef _WIN32
#  include <windows.h>
#  define MAXPATHLEN MAX_PATH
#  include <direct.h> // getcwd
#  define snprintf _snprintf
#else
#  include <sys/param.h> // MAXPATHLEN
#  include <unistd.h> // getcwd
#endif
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include <AR/config.h>
#include <AR/video.h>
#include <AR/param.h>			// arParamDisp(), arParamSaveOptical()
#include <AR/ar.h>
#include <AR/gsub_lite.h>
#include <ARUtil/time.h>
#include "calc_optical.h"
#include "getInput.h"
#include <Eden/EdenGLFont.h>
#ifndef EDEN_OPENGLES
#  define DISABLE_GL_STATE_CACHE
#endif
#include "glStateCache.h"

// ============================================================================
//	Constants
// ============================================================================

#define FONT_SIZE 12.0f
#define FONT_LINE_SPACING 1.125f

#undef STEREO_SUPPORT_MODES_REQUIRING_STENCIL // Not enabled as full-window stencilling is very slow on most hardware.

// ============================================================================
//	Global variables
// ============================================================================

// Preferences.
static int prefWindowed = FALSE;          // Use windowed (TRUE) or fullscreen mode (FALSE) on launch.
static int prefWidth = 0;                 // Preferred initial window width.
static int prefHeight = 0;                // Preferred initial window height.
static int prefDepth = 0;                 // Fullscreen mode bit depth. Set to 0 to use default depth.
static int prefRefresh = 0;				  // Fullscreen mode refresh rate. Set to 0 to use default rate.

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

static VIEW_EYE_t calibrationEye = VIEW_LEFTEYE;
#define  CALIB_POS1_NUM     5	// Use 5 positions in the display image plane (x-y)
#define  CALIB_POS2_NUM     2	// Use 2 positions in the display depth axis (z)
// Centre of the four quadrants, and centre of whole image.
static double	calib_pos[CALIB_POS1_NUM][2];
static double   calib_pos2d[CALIB_POS1_NUM][CALIB_POS2_NUM][2];
static double   calib_pos3d[CALIB_POS1_NUM][CALIB_POS2_NUM][3];
static int      co1;	// Index into which x-y coordinate we are capturing.
static int      co2;	// Index into which z coordinate we are capturing.

// Calibration mode.
typedef enum {
    MODE_CALIBRATION_NOT_STARTED,
    MODE_CALIBRATION_IN_PROGRESS,
    MODE_CALIBRATION_COMPLETE_GETTING_FILENAME,
    MODE_CALIBRATION_COMPLETED
} calib_mode_t;
static calib_mode_t calibMode = MODE_CALIBRATION_NOT_STARTED;
static char *calibModeCompleteMessage = NULL;

static char calibFilenameMono[] = "optical_param.dat";
static char calibFilenameStereoL[] = "optical_param_left.dat";
static char calibFilenameStereoR[] = "optical_param_right.dat";
static char *calibFilenameDefault = NULL;

// ============================================================================
//	Function prototypes.
// ============================================================================

static void usage(char *com);
static int setupCamera(const char *cparam_name, char *vconf, ARParamLT **cparamLT_p);
static int setupMarker(const char *patt_name, int *patt_id, ARPattHandle **pattHandle_p);
static void cleanup(void);
static void Keyboard(unsigned char key, int x, int y);
static void calibModeNext(void);
static void Visibility(int visible);
static void Reshape(int w, int h);
static void Display(void);
static void DisplayPerContext(const int contextIndex);
static void drawBackground(const float width, const float height, const float x, const float y, const bool drawBorder);
static void printHelpKeys(const int contextIndex);
static void printMode(const int contextIndex);
static void printCalibMode(const int contextIndex);

#ifdef _WIN32
int asprintf(char **ret, const char *format, ...)
{
	va_list ap;
	int len;

	va_start(ap, format);
    len = _vscprintf(format, ap);
    if (len >= 0) {
        *ret = (char *)malloc((len + 1)*sizeof(char)); // +1 for nul-term.
        vsprintf(*ret, format, ap);
    }
	va_end(ap);
	return (len);
}
#endif

// ============================================================================
//	Functions
// ============================================================================

int main(int argc, char** argv)
{
	char    glutGamemode[32] = "";
    char   *vconf = NULL;
    char    cparaDefault[] = "../share/calib_optical/Data/camera_para.dat";
    char   *cpara = NULL;
    int     i;
    int     gotTwoPartOption;
	char    patt_name[]  = "../share/calib_optical/Data/calib.patt";
	
    //
	// Process command-line options.
	//
    
	glutInit(&argc, argv);
    
    i = 1; // argv[0] is name of app, so start at 1.
    while (i < argc) {
        gotTwoPartOption = FALSE;
        // Look for two-part options first.
        if ((i + 1) < argc) {
            if (strcmp(argv[i], "--vconf") == 0) {
                i++;
                vconf = argv[i];
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--cpara") == 0) {
                i++;
                cpara = argv[i];
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
            } else if (strncmp(argv[i], "-cpara=", 7) == 0) {
                cpara = &(argv[i][7]);
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
    
	if (!setupCamera((cpara ? cpara : cparaDefault), vconf, &gCparamLT)) {
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
    
    gARHandle = arCreateHandle(gCparamLT);
    if (!gARHandle) {
        ARLOGe("Error creating AR handle.\n");
		exit(-1);
    }
    arPattAttach(gARHandle, gARPattHandle);
    
    if (arSetPixelFormat(gARHandle, arVideoGetPixelFormat())) {
        ARLOGe("Error setting pixel format.\n");
		exit(-1);
    }
    
    gAR3DHandle = ar3DCreateHandle(&gCparamLT->param);
    if (!gAR3DHandle) {
        ARLOGe("Error creating 3D handle.\n");
		exit(-1);
    }
    
    //
    // Markers setup.
    //
    
    // Load marker(s).
	if (!setupMarker(patt_name, &gPatt_id, &gARPattHandle)) {
		ARLOGe("main(): Unable to set up AR marker.\n");
		cleanup();
		exit(-1);
	}
    
    //
    // Other ARToolKit setup.
    //
    
    //arSetMarkerExtractionMode(gARHandle, AR_USE_TRACKING_HISTORY_V2);
    arSetMarkerExtractionMode(gARHandle, AR_NOUSE_TRACKING_HISTORY);
    //arSetLabelingThreshMode(gARHandle, AR_LABELING_THRESH_MODE_MANUAL); // Uncomment to force manual thresholding.
    
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
        else glutInitWindowSize(gCparamLT->param.xsize, gCparamLT->param.ysize);
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

        // Setup ARgsub_lite library for current OpenGL context.
        if ((viewContexts[i].arglSettings = arglSetupForCurrentContext(&(gCparamLT->param), arVideoGetPixelFormat())) == NULL) {
            ARLOGe("main(): arglSetupForCurrentContext() returned error.\n");
            cleanup();
            exit(-1);
        }
        arglSetupDebugMode(viewContexts[i].arglSettings, gARHandle);
    }
    
    // Font setup.
    EdenGLFontInit(viewContextCount);
    EdenGLFontSetSize(FONT_SIZE);
    EdenGLFontSetFont(EDEN_GL_FONT_ID_Stroke_Roman);
    EdenGLFontSetWordSpacing(0.8f);
    //EdenGLFontSetLineSpacing(FONT_LINE_SPACING);
	
    //
    // Setup complete. Start tracking.
    //
    
    // Start the video.
	if (arVideoCapStart() != 0) {
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
    ARLOG("  --vconf <video parameter for the camera>\n");
    ARLOG("  --cpara <camera parameter file for the camera>\n");
    ARLOG("  -cpara=<camera parameter file for the camera>\n");
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

static int setupCamera(const char *cparam_name, char *vconf, ARParamLT **cparamLT_p)
{	
    ARParam			cparam;
	int				xsize, ysize;
    AR_PIXEL_FORMAT pixFormat;

    // Open the video path.
    if (arVideoOpen(vconf) < 0) {
    	ARLOGe("setupCamera(): Unable to open connection to camera.\n");
    	return (FALSE);
	}
	
    // Find the size of the window.
    if (arVideoGetSize(&xsize, &ysize) < 0) {
        ARLOGe("setupCamera(): Unable to determine camera frame size.\n");
        arVideoClose();
        return (FALSE);
    }
    ARLOG("Camera image size (x,y) = (%d,%d)\n", xsize, ysize);
	
	// Get the format in which the camera is returning pixels.
	pixFormat = arVideoGetPixelFormat();
	if (pixFormat == AR_PIXEL_FORMAT_INVALID) {
    	ARLOGe("setupCamera(): Camera is using unsupported pixel format.\n");
        arVideoClose();
		return (FALSE);
	}
	
	// Load the camera parameters, resize for the window and init.
    if (arParamLoad(cparam_name, 1, &cparam) < 0) {
		ARLOGe("setupCamera(): Error loading parameter file %s for camera.\n", cparam_name);
        arVideoClose();
        return (FALSE);
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
        arVideoClose();
        return (FALSE);
    }

	return (TRUE);
}

static int setupMarker(const char *patt_name, int *patt_id, ARPattHandle **pattHandle_p)
{
	// Loading only 1 pattern in this example.
	if ((*patt_id = arPattLoad(*pattHandle_p, patt_name)) < 0) {
		ARLOGe("setupMarker(): Error loading pattern file %s.\n", patt_name);
		return (FALSE);
	}
	
	return (TRUE);
}

static void cleanup(void)
{
    int i;
    
    EdenGLFontFinal();

#ifdef STEREO_SUPPORT_MODES_REQUIRING_STENCIL
    if (stereoStencilPattern) free(stereoStencilPattern);
#endif
    if (calibModeCompleteMessage) free(calibModeCompleteMessage);

    if (viewContexts) {
        for (i = 0; i < viewContextCount; i++) {
            if (viewContexts[i].arglSettings) arglCleanup(viewContexts[i].arglSettings);
            free(viewContexts[i].views);
        }
        free(viewContexts);
        viewContextCount = 0;
    }
    if (views) {
        free(views);
        viewCount = 0;
    }
    
	arPattDetach(gARHandle);
	arPattDeleteHandle(gARPattHandle);
	arVideoCapStop();
	ar3DDeleteHandle(&gAR3DHandle);
	arDeleteHandle(gARHandle);
    arParamLTFree(&gCparamLT);
	arVideoClose();
}

static void calibModeNext(void)
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
	ARdouble fovy, aspect, m[16];
    char *cwd, *name;
    size_t len;
    char calibCompleteStringBadCalc[] = "ERROR: calculation of optical display parameters and transformation matrix failed!";
    char calibCompleteStringSaveError[] = "ERROR: attempt to save to file '%s/%s' failed!";
    char calibCompleteStringSaveOK[] = "Optical display parameters and transformation matrix saved to file '%s/%s'.";
    
	
    if (calibMode == MODE_CALIBRATION_NOT_STARTED || calibMode == MODE_CALIBRATION_COMPLETED) {
        
		ARLOG("Beginning optical see-through calibration.\n");
        
		// Adjust the on-screen calibration positions from normalised positions to the actual size we have.
        for (viewIndex = 0; views[viewIndex].viewEye != calibrationEye; viewIndex++); // Locate the view we're calibrating.
        contentWidth = (ARdouble)(views[viewIndex].contentWidth);
        contentHeight = (ARdouble)(views[viewIndex].contentHeight);
		for (i = 0; i < CALIB_POS1_NUM; i++) {
			calib_pos[i][0] = calib_posn_norm[i][0] * contentWidth;
			calib_pos[i][1] = calib_posn_norm[i][1] * contentHeight;
		}
		
		co1 = 0;
		co2 = 0;
        
        calibMode = MODE_CALIBRATION_IN_PROGRESS;
        
    } else if (calibMode == MODE_CALIBRATION_IN_PROGRESS) {
        
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
			if (co2 == CALIB_POS2_NUM) {
				co2 = 0;
				co1++;
				if (co1 == CALIB_POS1_NUM) {
                    
                    // All positions captured. Now need to get filename.
                    
                    if (stereoDisplayMode == STEREO_DISPLAY_MODE_INACTIVE) calibFilenameDefault = calibFilenameMono;
                    else {
                        if (calibrationEye == VIEW_RIGHTEYE) calibFilenameDefault = calibFilenameStereoR;
                        else calibFilenameDefault = calibFilenameStereoL;
                    }
                    
                    if (!getInputStart((unsigned char *)"> ", 0, MAXPATHLEN, 0, 0, 0)) {
                        ARLOGe("Error entering user input mode.\n");
                        cleanup();
                        exit (-1);
                    }
                    calibMode = MODE_CALIBRATION_COMPLETE_GETTING_FILENAME;
				}
			}
		} // gPatt_found
		
    } else if (calibMode == MODE_CALIBRATION_COMPLETE_GETTING_FILENAME) {
        
        // Input etc. handled in Keyboard().
        
        if (getInputIsComplete()) {
            
            name = (char *)getInput();
            if (!name) {
                
                // User cancelled.
                calibMode = MODE_CALIBRATION_NOT_STARTED;
                
            } else {
                
                // User supplied some text.
                
                if (calibModeCompleteMessage) {
                    free(calibModeCompleteMessage);
                    calibModeCompleteMessage = NULL;
                }
                
                // Calculate the params.
                if (calc_optical((ARdouble (*)[3])calib_pos3d, (ARdouble (*)[2])calib_pos2d, CALIB_POS1_NUM*CALIB_POS2_NUM,
                                 &fovy, &aspect, m) < 0) {
                    
                    // Params not OK. Show a message.
                    calibModeCompleteMessage = strdup(calibCompleteStringBadCalc);
                    
                } else {
                    
                    // Params OK. Save the file.
                    arMalloc(cwd, char, MAXPATHLEN);
                    if (!getcwd(cwd, MAXPATHLEN)) ARLOGe("Unable to read current working directory.\n");
                    len = strlen(name);
                    if (len < 1) name = calibFilenameDefault;
                    
                    if (arParamSaveOptical(name, fovy, aspect, m) < 0) {
                        len = sizeof(calibCompleteStringSaveError) + strlen(name) + strlen(cwd);
                        arMalloc(calibModeCompleteMessage, char, len);
                        snprintf(calibModeCompleteMessage, len, calibCompleteStringSaveError,  cwd, name);
                        ARLOG(calibCompleteStringSaveError, cwd, name);
                    } else {
                        len = sizeof(calibCompleteStringSaveOK) + strlen(name) + strlen(cwd);
                        arMalloc(calibModeCompleteMessage, char, len);
                        snprintf(calibModeCompleteMessage, len, calibCompleteStringSaveOK,  cwd, name);
                        ARLOG(calibCompleteStringSaveOK, cwd, name);
                    }
                    ARLOG("\n");
                    
                    free(cwd);
                }
                
                calibMode = MODE_CALIBRATION_COMPLETED;
            }
            
            getInputFinish();
        } // getInputIsComplete()
    }
}

static void Keyboard(unsigned char key, int x, int y)
{
	int mode, threshChange = 0;
    AR_LABELING_THRESH_MODE modea;
	
    if (calibMode == MODE_CALIBRATION_COMPLETE_GETTING_FILENAME) {
        
        getInputProcessKey(key);
        calibModeNext();

    } else {
        switch (key) {
            case 0x1B:						// Quit.
            case 'Q':
            case 'q':
                if (calibMode == MODE_CALIBRATION_IN_PROGRESS) {
                    calibMode = MODE_CALIBRATION_NOT_STARTED;
                } else {
                    cleanup();
                    exit(0);
                }
                break;
            case 'X':
            case 'x':
                arGetImageProcMode(gARHandle, &mode);
                switch (mode) {
                    case AR_IMAGE_PROC_FRAME_IMAGE:  mode = AR_IMAGE_PROC_FIELD_IMAGE; break;
                    case AR_IMAGE_PROC_FIELD_IMAGE:
                    default: mode = AR_IMAGE_PROC_FRAME_IMAGE; break;
                }
                arSetImageProcMode(gARHandle, mode);
                break;
            case 'C':
            case 'c':
                ARLOGe("*** Camera - %f (frame/sec)\n", (double)gCallCountMarkerDetect/arUtilTimer());
                gCallCountMarkerDetect = 0;
                arUtilTimerReset();
                break;
            case 'a':
            case 'A':
                arGetLabelingThreshMode(gARHandle, &modea);
                switch (modea) {
                    case AR_LABELING_THRESH_MODE_MANUAL:        modea = AR_LABELING_THRESH_MODE_AUTO_MEDIAN; break;
                    case AR_LABELING_THRESH_MODE_AUTO_MEDIAN:   modea = AR_LABELING_THRESH_MODE_AUTO_OTSU; break;
                    case AR_LABELING_THRESH_MODE_AUTO_OTSU:     modea = AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE; break;
                    case AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE: modea = AR_LABELING_THRESH_MODE_AUTO_BRACKETING; break;
                    case AR_LABELING_THRESH_MODE_AUTO_BRACKETING:
                    default: modea = AR_LABELING_THRESH_MODE_MANUAL; break;
                }
                arSetLabelingThreshMode(gARHandle, modea);
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
                arGetDebugMode(gARHandle, &mode);
                arSetDebugMode(gARHandle, !mode);
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
            case ' ':
                calibModeNext();
                break;
            case 'e':
            case 'E':
                if (stereoDisplayMode != STEREO_DISPLAY_MODE_INACTIVE) {
                    calibrationEye = (calibrationEye == VIEW_LEFTEYE ? VIEW_RIGHTEYE : VIEW_LEFTEYE);
                }
                break;
            default:
                break;
        }
        if (threshChange) {
            int threshhold;
            arGetLabelingThresh(gARHandle, &threshhold);
            threshhold += threshChange;
            if (threshhold < 0) threshhold = 0;
            if (threshhold > 255) threshhold = 255;
            arSetLabelingThresh(gARHandle, threshhold);
        }
    }
}

static void mainLoop(void)
{
	AR2VideoBufferT *image;
	ARdouble err;

    int             i, j, k;
	
	// Grab a video frame.
    image = arVideoGetImage();
    if (image && image->fillFlag){
        
        // Upload the image to all view contexts.
        if (gVideoSeeThrough) {
            for (i = 0; i < viewContextCount; i++) {
                arglPixelBufferDataUpload(viewContexts[i].arglSettings, image->buff);
            }
        }
		
		gCallCountMarkerDetect++; // Increment ARToolKit FPS counter.
		
		// Detect the markers in the video frame.
		if (arDetectMarker(gARHandle, image) < 0) {
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
			gPatt_found = TRUE;
		} else {
			gPatt_found = FALSE;
		}
		
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
    EdenGLFontSetViewSize(contentWidth, contentHeight);
    
	ARLOGe("Window was resized to %dx%d.\n", w, h);
}


static void drawLineSeg(double x1, double y1, double x2, double y2)
{
    glBegin(GL_LINES);
	glVertex2d(x1, y1);
	glVertex2d(x2, y2);
    glEnd();
}

static int drawAttention(const double posx, const double posy, const double side, const int color)
{
	double sided2 = side / 2.0;
	
    switch(color%7) {
		case 0: glColor3f(1.0f, 0.0f, 0.0f); break;
		case 1: glColor3f(0.0f, 1.0f, 0.0f); break;
		case 2: glColor3f(0.0f, 0.0f, 1.0f); break;
		case 3: glColor3f(1.0f, 1.0f, 0.0f); break;
		case 4: glColor3f(1.0f, 0.0f, 1.0f); break;
		case 5: glColor3f(0.0f, 1.0f, 1.0f); break;
		case 6: glColor3f(1.0f, 1.0f, 1.0f); break;
    }
	
    glLineWidth(5.0f);
    drawLineSeg(posx - sided2, posy - sided2, posx + sided2, posy - sided2);
    drawLineSeg(posx - sided2, posy + sided2, posx + sided2, posy + sided2);
    drawLineSeg(posx - sided2, posy - sided2, posx - sided2, posy + sided2);
    drawLineSeg(posx + sided2, posy - sided2, posx + sided2, posy + sided2);
    glLineWidth(1.0f);
	
    return(0);
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
	
        if (gVideoSeeThrough) {
            arglDispImage(viewContexts[contextIndex].arglSettings); // Retain 1:1 scaling, since the texture size will automatically be scaled to the viewport.
        }
        
        // Only 2D in this app.
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, view->contentWidth, 0.0, view->contentHeight, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
		
        // Any 2D overlays go here.
        
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        
        if (calibMode == MODE_CALIBRATION_IN_PROGRESS && viewEye == calibrationEye) {
            // Draw cross hairs.
            glLineWidth(3.0f);
            glColor3f(1.0f, 1.0f, 1.0f);
            drawLineSeg(0.0, calib_pos[co1][1], (double)(view->contentWidth), calib_pos[co1][1]);
            drawLineSeg(calib_pos[co1][0], 0.0, calib_pos[co1][0], (double)(view->contentHeight));
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
        if (gShowMode) {
            printMode(contextIndex);
        }
        if (gShowHelp) {
            if (gShowHelp == 1) {
                printHelpKeys(contextIndex);
            }
        }

        printCalibMode(contextIndex);
        
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

    } // end of view.
    
    if (stereoDisplayMode == STEREO_DISPLAY_MODE_FRAME_SEQUENTIAL) {
        stereoDisplaySequentialNext = (stereoDisplaySequentialNext == VIEW_LEFTEYE ? VIEW_RIGHTEYE : VIEW_LEFTEYE);
    }
}

//
// The following functions provide the onscreen help text and mode info.
//

static void drawBackground(const float width, const float height, const float x, const float y, const bool drawBorder)
{
    GLfloat vertices[4][2];
    
    vertices[0][0] = x; vertices[0][1] = y;
    vertices[1][0] = width + x; vertices[1][1] = y;
    vertices[2][0] = width + x; vertices[2][1] = height + y;
    vertices[3][0] = x; vertices[3][1] = height + y;
    
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
#ifdef GL_VERSION_1_3
    glClientActiveTexture(GL_TEXTURE0);
#endif
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);	// 50% transparent black.
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisable(GL_BLEND);
    if (drawBorder) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Opaque white.
        glLineWidth(1.0f);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }
    
    glPopClientAttrib();
}

static void printHelpKeys(const int contextIndex)
{
    GLfloat  bw, bh;
    const char *helpText[] = {
        "Keys:\n",
        " ? or /        Show/hide this help.",
        " q or [esc]    Quit program.",
        " o             Toggle optical / video see-through display.",
        " e             For stereo displays, toggle eye being calibrated.",
        " d             Activate / deactivate debug mode.",
        " m             Toggle display of mode info.",
        " a             Toggle between available threshold modes.",
        " - and +       Switch to manual threshold mode, and adjust threshhold up/down by 5.",
        " x             Change image processing mode.",
        " c             Calculate frame rate.",
    };
#define helpTextLineCount (sizeof(helpText)/sizeof(char *))
	float hMargin = 2.0f;
	float vMargin = 2.0f;
    

    bw = EdenGLFontGetBlockWidth((const unsigned char **)helpText, helpTextLineCount);
    bh = EdenGLFontGetBlockHeight((const unsigned char **)helpText, helpTextLineCount);
    drawBackground(bw, bh, hMargin, vMargin, false);

    glColor4ub(255, 255, 255, 255);
    EdenGLFontDrawBlock(contextIndex, (const unsigned char **)helpText, helpTextLineCount, hMargin, vMargin, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE);
}

static void printMode(const int contextIndex)
{
    int len, thresh, line, mode, xsize, ysize;
    AR_LABELING_THRESH_MODE threshMode;
    ARdouble tempF;
    int i;
    char *lines[256] = {NULL};
    unsigned char lineCount = 0;
    char text[256], *text_p;
	float hMargin = 2.0f;
	float vMargin = 2.0f;
    
    glColor4ub(255, 255, 255, 255);
    line = 1;
    
    // Image size and processing mode.
    arVideoGetSize(&xsize, &ysize);
    arGetImageProcMode(gARHandle, &mode);
	if (mode == AR_IMAGE_PROC_FRAME_IMAGE) text_p = "full frame";
	else text_p = "even field only";
    asprintf(&(lines[lineCount++]), "Processing %dx%d video frames %s", xsize, ysize, text_p);
    
    // Threshold mode, and threshold, if applicable.
    arGetLabelingThreshMode(gARHandle, &threshMode);
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
        arGetLabelingThresh(gARHandle, &thresh);
        len = (int)strlen(text);
        snprintf(text + len, sizeof(text) - len, ", thresh=%d", thresh);
    }
    lines[lineCount++] = strdup(text);
    
    // Border size, image processing mode, pattern detection mode.
    arGetBorderSize(gARHandle, &tempF);
    snprintf(text, sizeof(text), "Border: %0.1f%%", tempF*100.0);
    arGetPatternDetectionMode(gARHandle, &mode);
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
    lines[lineCount++] = strdup(text);
    
    EdenGLFontDrawBlock(contextIndex, (const unsigned char **)lines, lineCount, hMargin, vMargin, H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE, V_OFFSET_VIEW_TEXT_TOP_TO_VIEW_TOP);
    
    for (i = 0; i < lineCount; i++) free(lines[i]);
}

static void printCalibMode(const int contextIndex)
{
    int i;
    char *lines[256] = {NULL};
    unsigned char lineCount = 0;
    
	char outString[256] = "";
	GLint line = 0;
	float hMargin = 2.0f;
	float vMargin = 2.0f;
	
    glColor4ub(255, 255, 255, 255);
	
	if (calibMode == MODE_CALIBRATION_NOT_STARTED) {
        asprintf(&(lines[lineCount++]), "Optical see-through calibration: press space-bar to begin.");
        if (stereoDisplayMode != STEREO_DISPLAY_MODE_INACTIVE) {
            asprintf(&(lines[lineCount++]), "Calibration will be calculated for the %s eye.", (calibrationEye == VIEW_LEFTEYE ? "LEFT" : "RIGHT"));
        }
	} else if (calibMode == MODE_CALIBRATION_IN_PROGRESS) {
		asprintf(&(lines[lineCount++]), "Optical see-through calibration: position %d (%s).", co1 + 1, (co2 == 1 ? "near" : "far"));
		lineCount++;
		asprintf(&(lines[lineCount++]), "Hold the optical calibration marker in view of the camera.");
		asprintf(&(lines[lineCount++]), "(A coloured box will show when marker is in camera's view.)");
		asprintf(&(lines[lineCount++]), "Hold the marker as %s as possible, and", (co2 == 1 ? "near" : "far"));
		asprintf(&(lines[lineCount++]), "visually align the centre of the marker with the crosshairs on the display");
		asprintf(&(lines[lineCount++]), "and press the space bar to capture the position, or [esc] to cancel.");
		if (gPatt_found) {
			asprintf(&(lines[lineCount++]), "(x, y z) = (%5f, %5f, %5f)", gPatt_trans[0][3], gPatt_trans[1][3], gPatt_trans[2][3]);
            EdenGLFontDrawLine(contextIndex, (unsigned char *)outString, hMargin, vMargin + (EdenGLFontGetHeight() * EdenGLFontGetLineSpacing())*line++, H_OFFSET_TEXT_RIGHT_EDGE_TO_VIEW_RIGHT_EDGE, V_OFFSET_VIEW_TEXT_TOP_TO_VIEW_TOP);
            /*
            asprintf(&(lines[lineCount++]), "r (x, y z) = (%5f, %5f, %5f)", gPatt_trans[0][0], gPatt_trans[1][0], gPatt_trans[2][0]);
            asprintf(&(lines[lineCount++]), "u (x, y z) = (%5f, %5f, %5f)", gPatt_trans[0][1], gPatt_trans[1][1], gPatt_trans[2][1]);
            asprintf(&(lines[lineCount++]), "n (x, y z) = (%5f, %5f, %5f)", gPatt_trans[0][2], gPatt_trans[1][2], gPatt_trans[2][2]);
             */
		}
	} else if (calibMode == MODE_CALIBRATION_COMPLETE_GETTING_FILENAME) {
        asprintf(&(lines[lineCount++]), "Optical see-through calibration. Enter filename to save optical parameters to,");
        asprintf(&(lines[lineCount++]), "or press [return] to use default name ('%s'), or press [esc] to cancel.", calibFilenameDefault);
        asprintf(&(lines[lineCount++]), "%s", getInputPromptAndInputAndCursor());
    } else if (calibMode == MODE_CALIBRATION_COMPLETED) {
		asprintf(&(lines[lineCount++]), "Optical see-through calibration: complete.");
        if (calibModeCompleteMessage) {
            lines[lineCount++] = strdup(calibModeCompleteMessage);
        }
        asprintf(&(lines[lineCount++]), "Press space-bar to begin again.");
    }
    
    EdenGLFontDrawBlock(contextIndex, (const unsigned char **)lines, lineCount, hMargin, vMargin, H_OFFSET_TEXT_RIGHT_EDGE_TO_VIEW_RIGHT_EDGE, V_OFFSET_VIEW_TEXT_TOP_TO_VIEW_TOP);
    
    for (i = 0; i < lineCount; i++) free(lines[i]);
}


