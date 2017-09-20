/*
 *  simpleMovie.c
 *
 *  Some code to demonstrate use of ARToolKit drawing a QuickTime movie on the Hiro marker.
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
 *  Copyright 2010-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Philip Lamb.
 *
 */


// ============================================================================
//	Includes
// ============================================================================

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#  define snprintf _snprintf
#  define _USE_MATH_DEFINES
#endif
#include <stdlib.h>					// malloc(), free()
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

// ============================================================================
//	Constants
// ============================================================================

#define VIEW_SCALEFACTOR		1.0         // Units received from ARToolKit tracking will be multiplied by this factor before being used in OpenGL drawing.
#define VIEW_DISTANCE_MIN		40.0        // Objects closer to the camera than this will not be displayed. OpenGL units.
#define VIEW_DISTANCE_MAX		10000.0     // Objects further away from the camera than this will not be displayed. OpenGL units.

// ============================================================================
//	Global variables
// ============================================================================

// Preferences.
static int windowed = TRUE;                     // Use windowed (TRUE) or fullscreen mode (FALSE) on launch.
static int windowWidth = 640;					// Initial window width, also updated during program execution.
static int windowHeight = 480;                  // Initial window height, also updated during program execution.
static int windowDepth = 32;					// Fullscreen mode bit depth.
static int windowRefresh = 0;					// Fullscreen mode refresh rate. Set to 0 to use default rate.

static int          gARTImageSavePlease = FALSE;

// Marker detection.
static ARHandle		*gARHandle = NULL;
static ARPattHandle	*gARPattHandle = NULL;
static long			gCallCountMarkerDetect = 0;

// Transformation matrix retrieval.
static AR3DHandle	*gAR3DHandle = NULL;
static ARdouble		gPatt_width     = 80.0;	// Per-marker, but we are using only 1 marker.
static ARdouble		gPatt_trans[3][4];		// Per-marker, but we are using only 1 marker.
static int			gPatt_found = FALSE;	// Per-marker, but we are using only 1 marker.
static int			gPatt_id;				// Per-marker, but we are using only 1 marker.
static int          useContPoseEstimation = FALSE;

// Drawing.
static ARParamLT *gCparamLT = NULL;
static ARGL_CONTEXT_SETTINGS_REF gArglSettings = NULL;
static int gShowHelp = 1;
static int gShowMode = 1;

// Movie acquisition and drawing.
static AR2VideoParamT *gMovieVideo = NULL;
static ARUint8 *gMovieImage = NULL;
static ARParam gMovieCparam;
static ARGL_CONTEXT_SETTINGS_REF gMovieArglSettings = NULL;

// ============================================================================
//	Function prototypes.
// ============================================================================

static void print(const char *text, const float x, const float y, int calculateXFromRightEdge, int calculateYFromTopEdge);
static void drawBackground(const float width, const float height, const float x, const float y);
static void printHelpKeys();
static void printMode();

// ============================================================================
//	Functions
// ============================================================================

static int setupCamera(const char *cparam_name, char *vconf, ARParamLT **cparamLT_p, ARHandle **arhandle, AR3DHandle **ar3dhandle)
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
    ARLOGi("Camera image size (x,y) = (%d,%d)\n", xsize, ysize);
	
	// Get the format in which the camera is returning pixels.
	pixFormat = arVideoGetPixelFormat();
	if (pixFormat == AR_PIXEL_FORMAT_INVALID) {
    	ARLOGe("setupCamera(): Camera is using unsupported pixel format.\n");
        arVideoClose();
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
        arVideoClose();
        return (FALSE);
    }

    if ((*arhandle = arCreateHandle(*cparamLT_p)) == NULL) {
        ARLOGe("setupCamera(): Error: arCreateHandle.\n");
        return (FALSE);
    }
    if (arSetPixelFormat(*arhandle, pixFormat) < 0) {
        ARLOGe("setupCamera(): Error: arSetPixelFormat.\n");
        return (FALSE);
    }
	if (arSetDebugMode(*arhandle, AR_DEBUG_DISABLE) < 0) {
        ARLOGe("setupCamera(): Error: arSetDebugMode.\n");
        return (FALSE);
    }
	if ((*ar3dhandle = ar3DCreateHandle(&cparam)) == NULL) {
        ARLOGe("setupCamera(): Error: ar3DCreateHandle.\n");
        return (FALSE);
    }
	
	if (arVideoCapStart() != 0) {
    	ARLOGe("setupCamera(): Unable to begin camera data capture.\n");
		return (FALSE);		
	}
	
	return (TRUE);
}

static int setupMarker(const char *patt_name, int *patt_id, ARHandle *arhandle, ARPattHandle **pattHandle_p)
{	
    if ((*pattHandle_p = arPattCreateHandle()) == NULL) {
        ARLOGe("setupMarker(): Error: arPattCreateHandle.\n");
        return (FALSE);
    }
    
	// Loading only 1 pattern in this example.
	if ((*patt_id = arPattLoad(*pattHandle_p, patt_name)) < 0) {
		ARLOGe("setupMarker(): Error loading pattern file %s.\n", patt_name);
		arPattDeleteHandle(*pattHandle_p);
		return (FALSE);
	}
    
    arPattAttach(arhandle, *pattHandle_p);
	
	return (TRUE);
}

static int setupMovie(const char *path)
{
    char *movieVconf;
    int len;
 	int				xsize, ysize;
    AR_PIXEL_FORMAT pixFormat;
    
    // Construct the vconf string.
    arMalloc(movieVconf, char, 2048); // 2Kb for URL.
    sprintf(movieVconf, "-module=AVFoundation -movie=\""); // Make sure we're using the AVFoundation video input.
    len = (int)strlen(movieVconf);
    strncat(movieVconf + len, path, 2048 - len - 1);
    len = (int)strlen(movieVconf);
    strncat(movieVconf + len, "\"", 2048 - len - 1); // Movie will start paused. It will be unpaused in mainLoop().

    // Open the movie.
    gMovieVideo = ar2VideoOpen(movieVconf);
    free(movieVconf);
    if (!gMovieVideo) {
    	ARLOGe("setupMovie(): Unable to open movie.\n");
    	return (FALSE);
	}
	
    // Find the size of the movie.
    if (ar2VideoGetSize(gMovieVideo, &xsize, &ysize) < 0) return (FALSE);
	
	// Get the pixel format of the movie.
	pixFormat = ar2VideoGetPixelFormat(gMovieVideo);
	if (pixFormat == AR_PIXEL_FORMAT_INVALID) {
    	ARLOGe("setupMovie(): Movie is using unsupported pixel format.\n");
		return (FALSE);
	}
	
    // Set up an ARParam object for the movie input.
    arParamClear(&gMovieCparam, xsize, ysize, AR_DIST_FUNCTION_VERSION_DEFAULT);
    
    // For convenience, we will use gsub_lite to draw the actual pixels. Set it up now.
    gMovieArglSettings = arglSetupForCurrentContext(&gMovieCparam, pixFormat);
    arglDistortionCompensationSet(gMovieArglSettings, 0);
    
    return (TRUE);
}

static void cleanup(void)
{
    // Dispose of movie drawing structures, and stop and close the movie file.
    arglCleanup(gMovieArglSettings);
    gMovieArglSettings = NULL;
    if (gMovieVideo) {
        ar2VideoCapStop(gMovieVideo);
        ar2VideoClose(gMovieVideo);
    }    
	arglCleanup(gArglSettings);
    gArglSettings = NULL;
	arPattDetach(gARHandle);
	arPattDeleteHandle(gARPattHandle);
	arVideoCapStop();
	ar3DDeleteHandle(&gAR3DHandle);
	arDeleteHandle(gARHandle);
    arParamLTFree(&gCparamLT);
	arVideoClose();
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
        case 's':
        case 'S':
            if (!gARTImageSavePlease) gARTImageSavePlease = TRUE;
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

static void mainLoop(void)
{
	static int ms_prev;
	int ms;
	float s_elapsed;
	AR2VideoBufferT *image;
    AR2VideoBufferT *movieBuffer;
	ARdouble err;

    int             j, k;
	
	// Find out how long since mainLoop() last ran.
	ms = glutGet(GLUT_ELAPSED_TIME);
	s_elapsed = (float)(ms - ms_prev) * 0.001f;
	if (s_elapsed < 0.01f) return; // Don't update more often than 100 Hz.
	ms_prev = ms;
	
	// Grab a movie frame (if available).
    if ((movieBuffer = ar2VideoGetImage(gMovieVideo)) != NULL) {
        if (movieBuffer->buff && movieBuffer->fillFlag)
            gMovieImage = movieBuffer->buff;
    }
    
	// Grab a video frame.
    image = arVideoGetImage();
	if (image && image->fillFlag) {
        
        arglPixelBufferDataUpload(gArglSettings, image->buff);

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
            if (gPatt_found && useContPoseEstimation) {
                err = arGetTransMatSquareCont(gAR3DHandle, &(gARHandle->markerInfo[k]), gPatt_trans, gPatt_width, gPatt_trans);
            } else {
                err = arGetTransMatSquare(gAR3DHandle, &(gARHandle->markerInfo[k]), gPatt_width, gPatt_trans);
                // Marker has appeared, so un-pause movie.
                ar2VideoCapStart(gMovieVideo);
            }
			gPatt_found = TRUE;
		} else {
            if (gPatt_found) {
                // Marker has disappeared, so pause movie.
                ar2VideoCapStop(gMovieVideo);
            }
			gPatt_found = FALSE;
		}
		
		// Tell GLUT the display has changed.
		glutPostRedisplay();
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
    windowWidth = w;
    windowHeight = h;
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	
	// Call through to anyone else who needs to know about window sizing here.
}

//
// This function is called when the window needs redrawing.
//
static void Display(void)
{
    ARdouble p[16];
	ARdouble m[16];
    double zoom;
	
	// Select correct buffer for this context.
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers for new frame.
	
    arglDispImage(gArglSettings);
				
	// Projection transformation.
	arglCameraFrustumRH(&(gCparamLT->param), VIEW_DISTANCE_MIN, VIEW_DISTANCE_MAX, p);
	glMatrixMode(GL_PROJECTION);
#ifdef ARDOUBLE_IS_FLOAT
    glLoadMatrixf(p);
#else
    glLoadMatrixd(p);
#endif
	glMatrixMode(GL_MODELVIEW);
		
	glEnable(GL_DEPTH_TEST);

	// Viewing transformation.
	glLoadIdentity();
	// Lighting and geometry that moves with the camera should go here.
	// (I.e. must be specified before viewing transformations.)
	//none
	
	if (gPatt_found) {
	
		// Calculate the camera position relative to the marker.
		// Replace VIEW_SCALEFACTOR with 1.0 to make one drawing unit equal to 1.0 ARToolKit units (usually millimeters).
		arglCameraViewRH((const ARdouble (*)[4])gPatt_trans, m, VIEW_SCALEFACTOR);
#ifdef ARDOUBLE_IS_FLOAT
        glLoadMatrixf(m);
#else
        glLoadMatrixd(m);
#endif

		// All lighting and geometry to be drawn relative to the marker goes here.
        
        // Draw the movie frame.
        if (gMovieImage) {
            glPushMatrix();
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Place movie in x-z plane instead of x-y plane.
            glTranslated(-gPatt_width*0.5, 0.0f, 0.0f); // Movie origin is at lower-left of movie frame. Place this at the edge of the marker .
            zoom = 1.0/gMovieCparam.xsize * gPatt_width; // Scale the movie frame so that it is the same width as the marker.
            arglPixelBufferDataUpload(gMovieArglSettings, gMovieImage);
            arglDispImageStateful(gMovieArglSettings); // Show the movie frame.
            glPopMatrix();
        }
	
	} // gPatt_found
	
	// Any 2D overlays go here.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (GLdouble)windowWidth, 0, (GLdouble)windowHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    //
    // Draw help text and mode.
    //
    if (gShowMode) {
        printMode();
    }
    if (gShowHelp) {
        if (gShowHelp == 1) {
            printHelpKeys();
        }
    }
	
	glutSwapBuffers();
}

int main(int argc, char** argv)
{
	char glutGamemode[32] = "";
	char *cparam_name = NULL;
	char vconf[] = "";
	char patt_name[]  = "Data/hiro.patt";
    
    char movieFileName[] = "Data/sample.mp4";
	
    //
	// Library inits.
	//

	glutInit(&argc, argv);

    arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST, NULL);
    
	//
	// Video setup.
	//

	if (!setupCamera(cparam_name, vconf, &gCparamLT, &gARHandle, &gAR3DHandle)) {
		ARLOGe("main(): Unable to set up AR camera.\n");
		exit(-1);
	}

	//
	// Graphics setup.
	//

	// Set up GL context(s) for OpenGL to draw into.
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	if (!windowed) {
		if (windowRefresh) sprintf(glutGamemode, "%ix%i:%i@%i", windowWidth, windowHeight, windowDepth, windowRefresh);
		else sprintf(glutGamemode, "%ix%i:%i", windowWidth, windowHeight, windowDepth);
		glutGameModeString(glutGamemode);
		glutEnterGameMode();
	} else {
		glutInitWindowSize(windowWidth, windowHeight);
		glutCreateWindow(argv[0]);
	}

	// Setup ARgsub_lite library for current OpenGL context.
	if ((gArglSettings = arglSetupForCurrentContext(&(gCparamLT->param), arVideoGetPixelFormat())) == NULL) {
		ARLOGe("main(): arglSetupForCurrentContext() returned error.\n");
		cleanup();
		exit(-1);
	}
    arglSetupDebugMode(gArglSettings, gARHandle);
	arUtilTimerReset();
		
	// Load marker(s).
	if (!setupMarker(patt_name, &gPatt_id, gARHandle, &gARPattHandle)) {
		ARLOGe("main(): Unable to set up AR marker.\n");
		cleanup();
		exit(-1);
	}
	
    //
    // Movie loading.
    //

    // In this example, we load a movie via a file URI, but other URIs allowed
    // by the AVFoundation module would also be fine.

    if (!setupMovie(movieFileName)) {
        ARLOGe("Error: Unable to open movie from file '%s'.\n", movieFileName);
        cleanup();
        exit(-1);
    }

	// Register GLUT event-handling callbacks.
	// NB: mainLoop() is registered by Visibility.
	glutDisplayFunc(Display);
	glutReshapeFunc(Reshape);
	glutVisibilityFunc(Visibility);
	glutKeyboardFunc(Keyboard);
	
	glutMainLoop();

	return (0);
}

//
// The following functions provide the onscreen help text and mode info.
//

static void print(const char *text, const float x, const float y, int calculateXFromRightEdge, int calculateYFromTopEdge)
{
    int i, len;
    GLfloat x0, y0;
    
    if (!text) return;
    
    if (calculateXFromRightEdge) {
        x0 = windowWidth - x - (float)glutBitmapLength(GLUT_BITMAP_HELVETICA_10, (const unsigned char *)text);
    } else {
        x0 = x;
    }
    if (calculateYFromTopEdge) {
        y0 = windowHeight - y - 10.0f;
    } else {
        y0 = y;
    }
    glRasterPos2f(x0, y0);
    
    len = (int)strlen(text);
    for (i = 0; i < len; i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, text[i]);
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
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);	// 50% transparent black.
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Opaque white.
    //glLineWidth(1.0f);
    //glDrawArrays(GL_LINE_LOOP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}

static void printHelpKeys()
{
    int i;
    GLfloat  w, bw, bh;
    const char *helpText[] = {
        "Keys:\n",
        " ? or /        Show/hide this help.",
        " q or [esc]    Quit program.",
        " d             Activate / deactivate debug mode.",
        " m             Toggle display of mode info.",
        " a             Toggle between available threshold modes.",
        " - and +       Switch to manual threshold mode, and adjust threshhold up/down by 5.",
        " x             Change image processing mode.",
        " c             Calulcate frame rate.",
    };
#define helpTextLineCount (sizeof(helpText)/sizeof(char *))
    
    bw = 0.0f;
    for (i = 0; i < helpTextLineCount; i++) {
        w = (float)glutBitmapLength(GLUT_BITMAP_HELVETICA_10, (unsigned char *)helpText[i]);
        if (w > bw) bw = w;
    }
    bh = helpTextLineCount * 10.0f /* character height */+ (helpTextLineCount - 1) * 2.0f /* line spacing */;
    drawBackground(bw, bh, 2.0f, 2.0f);
    
    for (i = 0; i < helpTextLineCount; i++) print(helpText[i], 2.0f, (helpTextLineCount - 1 - i)*12.0f + 2.0f, 0, 0);;
}

static void printMode()
{
    int len, thresh, line, mode, xsize, ysize;
    AR_LABELING_THRESH_MODE threshMode;
    ARdouble tempF;
    char text[256], *text_p;

    glColor3ub(255, 255, 255);
    line = 1;
    
    // Image size and processing mode.
    arVideoGetSize(&xsize, &ysize);
    arGetImageProcMode(gARHandle, &mode);
	if (mode == AR_IMAGE_PROC_FRAME_IMAGE) text_p = "full frame";
	else text_p = "even field only";
    snprintf(text, sizeof(text), "Processing %dx%d video frames %s", xsize, ysize, text_p);
    print(text, 2.0f,  (line - 1)*12.0f + 2.0f, 0, 1);
    line++;
    
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
    print(text, 2.0f,  (line - 1)*12.0f + 2.0f, 0, 1);
    line++;
    
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
    print(text, 2.0f,  (line - 1)*12.0f + 2.0f, 0, 1);
    line++;
    
    // Window size.
    snprintf(text, sizeof(text), "Drawing into %dx%d window", windowWidth, windowHeight);
    print(text, 2.0f,  (line - 1)*12.0f + 2.0f, 0, 1);
    line++;
    
}
