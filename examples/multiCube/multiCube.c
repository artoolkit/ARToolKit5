/*
 *  multiCube.c
 *
 *  Some code to demonstrate use of ARToolKit.
 *  This example demonstrates use of a cube-shaped multi marker.
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
 *  Copyright 2011-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Philip Lamb.
 *
 */


// ============================================================================
//	Includes
// ============================================================================

#include <stdio.h>
#include <stdlib.h>					// malloc(), free()
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#ifdef _WIN32
#  define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <AR/config.h>
#include <AR/video.h>
#include <AR/param.h>			// arParamDisp()
#include <AR/ar.h>
#include <AR/arMulti.h>
#include <AR/gsub_lite.h>
#include <ARUtil/time.h>

// ============================================================================
//	Constants
// ============================================================================

#define VIEW_SCALEFACTOR		1.0         // Use ARToolKit units (millimeters) for drawing.
#define VIEW_DISTANCE_MIN		5.0         // Objects closer to the camera than this will not be displayed.
#define VIEW_DISTANCE_MAX		3000.0		// Objects further away from the camera than this will not be displayed.

// ============================================================================
//	Global variables
// ============================================================================

// Preferences.
static int prefWindowed = TRUE;
static int prefWidth = 640;					// Fullscreen mode width.
static int prefHeight = 480;				// Fullscreen mode height.
static int prefDepth = 32;					// Fullscreen mode bit depth.
static int prefRefresh = 0;					// Fullscreen mode refresh rate. Set to 0 to use default rate.

// Marker detection.
static ARHandle		*gARHandle = NULL;
static ARPattHandle	*gARPattHandle = NULL;
static long			gCallCountMarkerDetect = 0;

// Transformation matrix retrieval.
static AR3DHandle	*gAR3DHandle = NULL;
static ARdouble		gPatt_trans[3][4];		// Per-marker, but we are using only 1 marker.
static int			gPatt_found = FALSE;	// Per-marker, but we are using only 1 marker.
static ARMultiMarkerInfoT *gMultiConfig;

// Drawing.
static ARParamLT *gCparamLT = NULL;
static ARGL_CONTEXT_SETTINGS_REF gArglSettings = NULL;
static int gDrawRotate = FALSE;
static float gDrawRotateAngle = 0;			// For use in drawing.


// ============================================================================
//	Functions
// ============================================================================

// Colour cube data.
static const GLfloat cube_vertices [8][3] = {
    /* +z */ {0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
    /* -z */ {0.5f, 0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f} };
static const GLubyte cube_vertex_colors [8][4] = {
    {255, 255, 255, 255}, {255, 255, 0, 255}, {0, 255, 0, 255}, {0, 255, 255, 255},
    {255, 0, 255, 255}, {255, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 255, 255} };
static const GLubyte cube_faces [6][4] = { /* ccw-winding */
    /* +z */ {3, 2, 1, 0}, /* -y */ {2, 3, 7, 6}, /* +y */ {0, 1, 5, 4},
    /* -x */ {3, 0, 4, 7}, /* +x */ {1, 2, 6, 5}, /* -z */ {4, 5, 6, 7} };

static void DrawCubeMarkerMask(void)
{
    int i;
    glPushMatrix(); // Save world coordinate system.
    glScalef(60.0f, 60.0f, 60.0f); // Cube marker is 60 mm each side.
    glTranslatef(0.0f, 0.0f, -0.5f); // Place top of cube on marker surface.
	glDisable(GL_LIGHTING);	// Just use colours.
    glDisable(GL_TEXTURE_2D);
    glColorMask(0, 0, 0, 0); // Masking, so draw only into z-buffer.
    glVertexPointer(3, GL_FLOAT, 0, cube_vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    for (i = 0; i < 6; i++) {
        glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_BYTE, &(cube_faces[i][0]));
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glColorMask(1, 1, 1, 1);
    glPopMatrix();	// Restore world coordinate system.        
}

// Something to look at, draw a rotating colour cube.
static void DrawCube(void)
{
    int i;
    
    glPushMatrix(); // Save world coordinate system.
    glScalef(30.0f, 30.0f, 30.0f); // Draw cube 30 mm each side.
    glTranslatef(0.0f, 0.0f, 0.5f); // Place base of cube on marker surface.
    glRotatef(gDrawRotateAngle, 0.0f, 0.0f, 1.0f); // Rotate about z axis.
	glDisable(GL_LIGHTING);	// Just use colours.
    glDisable(GL_TEXTURE_2D);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, cube_vertex_colors);
    glVertexPointer(3, GL_FLOAT, 0, cube_vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    for (i = 0; i < 6; i++) {
        glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_BYTE, &(cube_faces[i][0]));
    }
    glDisableClientState(GL_COLOR_ARRAY);
    glColor4ub(0, 0, 0, 1);
    for (i = 0; i < 6; i++) {
        glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_BYTE, &(cube_faces[i][0]));
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();	// Restore world coordinate system.    
}

static void DrawCubeUpdate(float timeDelta)
{
	if (gDrawRotate) {
		gDrawRotateAngle += timeDelta * 45.0f; // Rotate cube at 45 degrees per second.
		if (gDrawRotateAngle > 360.0f) gDrawRotateAngle -= 360.0f;
	}
}

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

static int setupCubeMarker(const char *patt_name, ARMultiMarkerInfoT **multiConfig_p, ARHandle *arhandle, ARPattHandle **pattHandle_p)
{	
    if ((*pattHandle_p = arPattCreateHandle()) == NULL) {
        ARLOGe("setupCubeMarker(): Error: arPattCreateHandle.\n");
        return (FALSE);
    }
    
	if (((*multiConfig_p) = arMultiReadConfigFile(patt_name, *pattHandle_p)) == NULL) {
		ARLOGe("setupCubeMarker(): Error reading multimarker config file '%s'.\n", patt_name);
		arPattDeleteHandle(*pattHandle_p);
		return (FALSE);
	}
	if( (*multiConfig_p)->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE ) {
        arSetPatternDetectionMode( arhandle, AR_TEMPLATE_MATCHING_COLOR );
    } else if( (*multiConfig_p)->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_MATRIX ) {
        arSetPatternDetectionMode( arhandle, AR_MATRIX_CODE_DETECTION );
    } else { // AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE_AND_MATRIX
        arSetPatternDetectionMode( arhandle, AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX );
    }	
    
    arPattAttach(arhandle, *pattHandle_p);
	
	return (TRUE);
}

// Report state of ARToolKit tracker.
static void debugReportMode(ARHandle *arhandle)
{
	int mode;
	
	arGetImageProcMode(arhandle, &mode);
	if (mode == AR_IMAGE_PROC_FRAME_IMAGE) {
		ARLOGe("ProcMode (X)   : FRAME IMAGE\n");
	} else if (mode == AR_IMAGE_PROC_FIELD_IMAGE) {
		ARLOGe("ProcMode (X)   : FIELD IMAGE\n");
	}
	
	arGetPatternDetectionMode(arhandle, &mode);
	if (mode == AR_TEMPLATE_MATCHING_COLOR) {
		ARLOGe("TemplateMatchingMode (M)   : Color Template\n");
	} else if (mode == AR_TEMPLATE_MATCHING_MONO) {
		ARLOGe("TemplateMatchingMode (M)   : Mono Template\n");
	} else if (mode == AR_MATRIX_CODE_DETECTION) {
		ARLOGe("TemplateMatchingMode (M)   : Matrix code detection\n");
	}
}

static void cleanup(void)
{
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
	
	switch (key) {
		case 0x1B:						// Quit.
		case 'Q':
		case 'q':
			cleanup();
			exit(0);
			break;
		case ' ':
			gDrawRotate = !gDrawRotate;
			break;
		case 'C':
		case 'c':
			ARLOGe("*** Camera - %f (frame/sec)\n", (double)gCallCountMarkerDetect/arUtilTimer());
			gCallCountMarkerDetect = 0;
			arUtilTimerReset();
			debugReportMode(gARHandle);
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
        case 'B':
        case 'b':
            arGetLabelingMode(gARHandle, &mode);
            if (mode == AR_LABELING_BLACK_REGION) arSetLabelingMode(gARHandle, AR_LABELING_WHITE_REGION);
            else arSetLabelingMode(gARHandle, AR_LABELING_BLACK_REGION);
			break;
        case '?':
		case '/':
			ARLOG("Keys:\n");
			ARLOG(" q or [esc]    Quit demo.\n");
			ARLOG(" c             Calulcate frame rate.\n");
			ARLOG(" - and +       Adjust threshhold.\n");
			ARLOG(" d             Activate / deactivate debug mode.\n");
			ARLOG(" b             Toggle between detection of black markers and white markers.\n");
			ARLOG(" ? or /        Show this help.\n");
			ARLOG("\nAdditionally, the ARVideo library supplied the following help text:\n");
			arVideoDispOption();
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
		ARLOG("Threshhold changed to %d.\n", threshhold);
	}
	
}

static void mainLoop(void)
{
	static int ms_prev;
	int ms;
	float s_elapsed;
	AR2VideoBufferT *image;
	ARdouble err;

    int             j, k;
	
	// Find out how long since mainLoop() last ran.
	ms = glutGet(GLUT_ELAPSED_TIME);
	s_elapsed = (float)(ms - ms_prev) * 0.001f;
	if (s_elapsed < 0.01f) return; // Don't update more often than 100 Hz.
	ms_prev = ms;
	
	// Update drawing.
	DrawCubeUpdate(s_elapsed);
	
	// Grab a video frame.
    image = arVideoGetImage();
	if (image && image->fillFlag) {
        arglPixelBufferDataUpload(gArglSettings, image->buff);
		
		gCallCountMarkerDetect++; // Increment ARToolKit FPS counter.
		
		// Detect the markers in the video frame.
		if (arDetectMarker(gARHandle, image) < 0) {
			exit(-1);
		}

		err = arGetTransMatMultiSquare( gAR3DHandle, arGetMarker( gARHandle ), arGetMarkerNum( gARHandle ), gMultiConfig);
		if( gMultiConfig->prevF != 0 ) {
			gPatt_found = TRUE;
			for (k = 0; k < 3; k++) {
				for (j = 0; j < 4; j++) {
					gPatt_trans[k][j] = gMultiConfig->trans[k][j];
				}
			}
		} else {
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Call through to anyone else who needs to know about window sizing here.
}

//
// This function is called when the window needs redrawing.
//
static void Display(void)
{
    ARdouble p[16];
	ARdouble m[16];
	
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

        // Before drawing any geometry, mask out the area occupied by the cube marker itself.
        // This makes for a nicer visual presentation, but obviously only applies for this particular
        // shape of marker.
        DrawCubeMarkerMask();
        
		// All lighting and geometry to be drawn relative to the marker goes here.
		DrawCube();
	
	} // gPatt_found
	
	// Any 2D overlays go here.
	//none
	
	glutSwapBuffers();
}

int main(int argc, char** argv)
{
	char glutGamemode[32];
	char *cparam_name = NULL;
    char vconf[] = "";
	const char *patt_name  = "Data/cubeMarkerConfig.dat";
	
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
	if (!prefWindowed) {
		if (prefRefresh) sprintf(glutGamemode, "%ix%i:%i@%i", prefWidth, prefHeight, prefDepth, prefRefresh);
		else sprintf(glutGamemode, "%ix%i:%i", prefWidth, prefHeight, prefDepth);
		glutGameModeString(glutGamemode);
		glutEnterGameMode();
	} else {
		glutInitWindowSize(prefWidth, prefHeight);
		glutCreateWindow(argv[0]);
	}

	// Setup ARgsub_lite library for current OpenGL context.
	if ((gArglSettings = arglSetupForCurrentContext(&(gCparamLT->param), arVideoGetPixelFormat())) == NULL) {
		ARLOGe("main(): arglSetupForCurrentContext() returned error.\n");
		cleanup();
		exit(-1);
	}
    arglSetupDebugMode(gArglSettings, gARHandle);
	debugReportMode(gARHandle);
	glEnable(GL_DEPTH_TEST);
	arUtilTimerReset();
		
	// Load marker(s).
	if (!setupCubeMarker(patt_name, &gMultiConfig, gARHandle, &gARPattHandle)) {
		ARLOGe("main(): Unable to set up AR marker.\n");
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
