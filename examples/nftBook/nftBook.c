/*
 *	nftBook.c
 *  ARToolKit5
 *
 *	Demonstration of ARToolKit NFT with models rendered in OSG,
 *  and marker pose estimates filtered to reduce jitter.
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

#ifdef _WIN32
#  include <windows.h>
#  define _USE_MATH_DEFINES
#  define snprintf _snprintf
#endif
#include <stdio.h>
#include <stdlib.h>					// malloc(), free()
#include <string.h>
#include <math.h>
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include <AR/ar.h>
#include <AR/arMulti.h>
#include <AR/video.h>
#include <AR/gsub_lite.h>
#include <AR2/tracking.h>
#include <ARUtil/time.h>

#include "ARMarkerNFT.h"
#include "trackingSub.h"
#include "VirtualEnvironment.h"

// ============================================================================
//	Constants
// ============================================================================

#define PAGES_MAX               10          // Maximum number of pages expected. You can change this down (to save memory) or up (to accomodate more pages.)

#define VIEW_SCALEFACTOR		1.0			// Units received from ARToolKit tracking will be multiplied by this factor before being used in OpenGL drawing.
#define VIEW_DISTANCE_MIN		10.0		// Objects closer to the camera than this will not be displayed. OpenGL units.
#define VIEW_DISTANCE_MAX		10000.0		// Objects further away from the camera than this will not be displayed. OpenGL units.

#define FONT_SIZE 10.0f
#define FONT_LINE_SPACING 2.0f

// ============================================================================
//	Global variables
// ============================================================================

// Preferences.
static int prefWindowed = TRUE;           // Use windowed (TRUE) or fullscreen mode (FALSE) on launch.
static int prefWidth = 640;               // Preferred initial window width.
static int prefHeight = 480;              // Preferred initial window height.
static int prefDepth = 32;                // Fullscreen mode bit depth. Set to 0 to use default depth.
static int prefRefresh = 0;				  // Fullscreen mode refresh rate. Set to 0 to use default rate.

// Markers.
static ARMarkerNFT *markersNFT = NULL;
static int markersNFTCount = 0;

// NFT.
static THREAD_HANDLE_T     *threadHandle = NULL;
static AR2HandleT          *ar2Handle = NULL;
static KpmHandle           *kpmHandle = NULL;
static int                  surfaceSetCount = 0;
static AR2SurfaceSetT      *surfaceSet[PAGES_MAX];
static long                 gCallCountMarkerDetect = 0;

// NFT results.
static int detectedPage = -2; // -2 Tracking not inited, -1 tracking inited OK, >= 0 tracking online on page.
static float trackingTrans[3][4];

// Drawing.
static int gWindowW;
static int gWindowH;
static ARParamLT *gCparamLT = NULL;
static ARGL_CONTEXT_SETTINGS_REF gArglSettings = NULL;
static double gFPS;
static ARdouble cameraLens[16];
static ARdouble cameraPose[16];
static int cameraPoseValid;


// ============================================================================
//	Function prototypes
// ============================================================================

static void usage(char *com);
static int setupCamera(const char *cparam_name, char *vconf, ARParamLT **cparamLT_p);
static int initNFT(ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat);
static int loadNFTData(void);
static void cleanup(void);
static void Keyboard(unsigned char key, int x, int y);
static void Visibility(int visible);
static void Reshape(int w, int h);
static void Display(void);

// ============================================================================
//	Functions
// ============================================================================

int main(int argc, char** argv)
{
    char    glutGamemode[32] = "";
    char   *vconf = NULL;
    char   *cparam_name = NULL;
    int     i;
    int     gotTwoPartOption;
    const char markerConfigDataFilename[] = "Data2/markers.dat";
    const char objectDataFilename[] = "Data2/objects.dat";

#ifdef DEBUG
    arLogLevel = AR_LOG_LEVEL_DEBUG;
#endif
    
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
            if (strcmp(argv[i], "--vconf") == 0) {
                i++;
                vconf = argv[i];
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--cpara") == 0) {
                i++;
                cparam_name = argv[i];
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
                cparam_name = &(argv[i][7]);
            } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
                ARLOG("%s version %s\n", argv[0], AR_HEADER_VERSION_STRING);
                exit(0);
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
    
    if (!setupCamera(cparam_name, vconf, &gCparamLT)) {
        ARLOGe("main(): Unable to set up AR camera.\n");
        exit(-1);
    }

    //
    // AR init.
    //
    
    if (!initNFT(gCparamLT, arVideoGetPixelFormat())) {
		ARLOGe("main(): Unable to init NFT.\n");
		exit(-1);
    }

    //
    // Markers setup.
    //
    
    // Load marker(s).
    newMarkers(markerConfigDataFilename, &markersNFT, &markersNFTCount);
    if (!markersNFTCount) {
        ARLOGe("Error loading markers from config. file '%s'.\n", markerConfigDataFilename);
		cleanup();
		exit(-1);
    }
    ARLOGi("Marker count = %d\n", markersNFTCount);
    
    // Marker data has been loaded, so now load NFT data.
    if (!loadNFTData()) {
        ARLOGe("Error loading NFT data.\n");
		cleanup();
		exit(-1);
    }
	
	//
	// Graphics setup.
	//

	// Set up GL context(s) for OpenGL to draw into.
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    if (prefWindowed) {
        if (prefWidth > 0 && prefHeight > 0) glutInitWindowSize(prefWidth, prefHeight);
        else glutInitWindowSize(gCparamLT->param.xsize, gCparamLT->param.ysize);
        glutCreateWindow(argv[0]);
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
        } else {
            if (prefWidth > 0 && prefHeight > 0) glutInitWindowSize(prefWidth, prefHeight);
            glutCreateWindow(argv[0]);
            glutFullScreen();
        }
    }
    
    // Create the OpenGL projection from the calibrated camera parameters.
    arglCameraFrustumRH(&(gCparamLT->param), VIEW_DISTANCE_MIN, VIEW_DISTANCE_MAX, cameraLens);
    cameraPoseValid = FALSE;
    
	// Setup ARgsub_lite library for current OpenGL context.
	if ((gArglSettings = arglSetupForCurrentContext(&(gCparamLT->param), arVideoGetPixelFormat())) == NULL) {
		ARLOGe("main(): arglSetupForCurrentContext() returned error.\n");
		cleanup();
		exit(-1);
	}
    
    // Load objects (i.e. OSG models).
    VirtualEnvironmentInit(objectDataFilename);
    VirtualEnvironmentHandleARViewUpdatedCameraLens(cameraLens);
    
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
	glutDisplayFunc(Display);
	glutReshapeFunc(Reshape);
	glutVisibilityFunc(Visibility);
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
	
	return (TRUE);
}

// Modifies globals: kpmHandle, ar2Handle.
static int initNFT(ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat)
{
    ARLOGd("Initialising NFT.\n");
    //
    // NFT init.
    //
    
    // KPM init.
    kpmHandle = kpmCreateHandle(cparamLT);
    if (!kpmHandle) {
        ARLOGe("Error: kpmCreateHandle.\n");
        return (FALSE);
    }
    //kpmSetProcMode( kpmHandle, KpmProcHalfSize );
    
    // AR2 init.
    if( (ar2Handle = ar2CreateHandle(cparamLT, pixFormat, AR2_TRACKING_DEFAULT_THREAD_NUM)) == NULL ) {
        ARLOGe("Error: ar2CreateHandle.\n");
        kpmDeleteHandle(&kpmHandle);
        return (FALSE);
    }
    if (threadGetCPU() <= 1) {
        ARLOGi("Using NFT tracking settings for a single CPU.\n");
        ar2SetTrackingThresh(ar2Handle, 5.0);
        ar2SetSimThresh(ar2Handle, 0.50);
        ar2SetSearchFeatureNum(ar2Handle, 16);
        ar2SetSearchSize(ar2Handle, 6);
        ar2SetTemplateSize1(ar2Handle, 6);
        ar2SetTemplateSize2(ar2Handle, 6);
    } else {
        ARLOGi("Using NFT tracking settings for more than one CPU.\n");
        ar2SetTrackingThresh(ar2Handle, 5.0);
        ar2SetSimThresh(ar2Handle, 0.50);
        ar2SetSearchFeatureNum(ar2Handle, 16);
        ar2SetSearchSize(ar2Handle, 12);
        ar2SetTemplateSize1(ar2Handle, 6);
        ar2SetTemplateSize2(ar2Handle, 6);
    }
    // NFT dataset loading will happen later.
    return (TRUE);
}

// Modifies globals: threadHandle, surfaceSet[], surfaceSetCount
static int unloadNFTData(void)
{
    int i, j;
    
    if (threadHandle) {
        ARLOGi("Stopping NFT2 tracking thread.\n");
        trackingInitQuit(&threadHandle);
    }
    j = 0;
    for (i = 0; i < surfaceSetCount; i++) {
        if (j == 0) ARLOGi("Unloading NFT tracking surfaces.\n");
        ar2FreeSurfaceSet(&surfaceSet[i]); // Also sets surfaceSet[i] to NULL.
        j++;
    }
    if (j > 0) ARLOGi("Unloaded %d NFT tracking surfaces.\n", j);
    surfaceSetCount = 0;
    
    return 0;
}

// References globals: markersNFTCount
// Modifies globals: threadHandle, surfaceSet[], surfaceSetCount, markersNFT[]
static int loadNFTData(void)
{
    int i;
	KpmRefDataSet *refDataSet;
    
    // If data was already loaded, stop KPM tracking thread and unload previously loaded data.
    if (threadHandle) {
        ARLOGi("Reloading NFT data.\n");
        unloadNFTData();
    } else {
        ARLOGi("Loading NFT data.\n");
    }
    
    refDataSet = NULL;
    
    for (i = 0; i < markersNFTCount; i++) {
        // Load KPM data.
        KpmRefDataSet  *refDataSet2;
        ARLOGi("Reading %s.fset3\n", markersNFT[i].datasetPathname);
        if (kpmLoadRefDataSet(markersNFT[i].datasetPathname, "fset3", &refDataSet2) < 0 ) {
            ARLOGe("Error reading KPM data from %s.fset3\n", markersNFT[i].datasetPathname);
            markersNFT[i].pageNo = -1;
            continue;
        }
        markersNFT[i].pageNo = surfaceSetCount;
        ARLOGi("  Assigned page no. %d.\n", surfaceSetCount);
        if (kpmChangePageNoOfRefDataSet(refDataSet2, KpmChangePageNoAllPages, surfaceSetCount) < 0) {
            ARLOGe("Error: kpmChangePageNoOfRefDataSet\n");
            exit(-1);
        }
        if (kpmMergeRefDataSet(&refDataSet, &refDataSet2) < 0) {
            ARLOGe("Error: kpmMergeRefDataSet\n");
            exit(-1);
        }
        ARLOGi("  Done.\n");
        
        // Load AR2 data.
        ARLOGi("Reading %s.fset\n", markersNFT[i].datasetPathname);
        
        if ((surfaceSet[surfaceSetCount] = ar2ReadSurfaceSet(markersNFT[i].datasetPathname, "fset", NULL)) == NULL ) {
            ARLOGe("Error reading data from %s.fset\n", markersNFT[i].datasetPathname);
        }
        ARLOGi("  Done.\n");
        
        surfaceSetCount++;
        if (surfaceSetCount == PAGES_MAX) break;
    }
    if (kpmSetRefDataSet(kpmHandle, refDataSet) < 0) {
        ARLOGe("Error: kpmSetRefDataSet\n");
        exit(-1);
    }
    kpmDeleteRefDataSet(&refDataSet);
    
    // Start the KPM tracking thread.
    threadHandle = trackingInitInit(kpmHandle);
    if (!threadHandle) exit(-1);

	ARLOGi("Loading of NFT data complete.\n");
    return (TRUE);
}

static void cleanup(void)
{
    VirtualEnvironmentFinal();

    if (markersNFT) deleteMarkers(&markersNFT, &markersNFTCount);
    
    // NFT cleanup.
    unloadNFTData();
	ARLOGd("Cleaning up ARToolKit NFT handles.\n");
    ar2DeleteHandle(&ar2Handle);
    kpmDeleteHandle(&kpmHandle);
    arParamLTFree(&gCparamLT);

    // OpenGL cleanup.
    arglCleanup(gArglSettings);
    gArglSettings = NULL;
    
    // Camera cleanup.
	arVideoCapStop();
	arVideoClose();
}

static void Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		case 0x1B:						// Quit.
		case 'Q':
		case 'q':
			cleanup();
			exit(0);
			break;
		case '?':
		case '/':
			ARLOG("Keys:\n");
			ARLOG(" q or [esc]    Quit demo.\n");
			ARLOG(" ? or /        Show this help.\n");
			ARLOG("\nAdditionally, the ARVideo library supplied the following help text:\n");
			arVideoDispOption();
			break;
		default:
			break;
	}
}

static void mainLoop(void)
{
	static int ms_prev;
	int ms;
	float s_elapsed;
	AR2VideoBufferT *image;


    int             i, j, k;
	
	// Calculate time delta.
	ms = glutGet(GLUT_ELAPSED_TIME);
	s_elapsed = (float)(ms - ms_prev) * 0.001f;
	ms_prev = ms;
	
	// Grab a video frame.
    image = arVideoGetImage();
	if (image && image->fillFlag) {
		
        arglPixelBufferDataUpload(gArglSettings, image->buff);

        // Calculate FPS every 30 frames.
        if (gCallCountMarkerDetect % 30 == 0) {
            gFPS = 30.0/arUtilTimer();
            arUtilTimerReset();
            gCallCountMarkerDetect = 0;
        }
		gCallCountMarkerDetect++; // Increment ARToolKit FPS counter.
		

        // Run marker detection on frame
        if (threadHandle) {
            // Perform NFT tracking.
            float            err;
            int              ret;
            int              pageNo;
            
            if( detectedPage == -2 ) {
                trackingInitStart( threadHandle, image->buffLuma );
                detectedPage = -1;
            }
            if( detectedPage == -1 ) {
                ret = trackingInitGetResult( threadHandle, trackingTrans, &pageNo);
                if( ret == 1 ) {
                    if (pageNo >= 0 && pageNo < surfaceSetCount) {
                        ARLOGd("Detected page %d.\n", pageNo);
                        detectedPage = pageNo;
                        ar2SetInitTrans(surfaceSet[detectedPage], trackingTrans);
                    } else {
                        ARLOGe("Detected bad page %d.\n", pageNo);
                        detectedPage = -2;
                    }
                } else if( ret < 0 ) {
                    ARLOGd("No page detected.\n");
                    detectedPage = -2;
                }
            }
            if( detectedPage >= 0 && detectedPage < surfaceSetCount) {
                if( ar2Tracking(ar2Handle, surfaceSet[detectedPage], image->buff, trackingTrans, &err) < 0 ) {
                    ARLOGd("Tracking lost.\n");
                    detectedPage = -2;
                } else {
                    ARLOGd("Tracked page %d (max %d).\n", detectedPage, surfaceSetCount - 1);
                }
            }
        } else {
            ARLOGe("Error: threadHandle\n");
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
                        ARLOGe("arFilterTransMat error with marker %d.\n", i);
                    }
                }
                
                if (!markersNFT[i].validPrev) {
                    // Marker has become visible, tell any dependent objects.
                    VirtualEnvironmentHandleARMarkerAppeared(i);
                }
                
                // We have a new pose, so set that.
                arglCameraViewRH((const ARdouble (*)[4])markersNFT[i].trans, markersNFT[i].pose.T, VIEW_SCALEFACTOR);
                // Tell any dependent objects about the update.
                VirtualEnvironmentHandleARMarkerWasUpdated(i, markersNFT[i].pose);
                
            } else {
                
                if (markersNFT[i].validPrev) {
                    // Marker has ceased to be visible, tell any dependent objects.
                    VirtualEnvironmentHandleARMarkerDisappeared(i);
                }
            }                    
        }

		// Tell GLUT the display has changed.
		glutPostRedisplay();
	} else {
		arUtilSleep(2);
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
    GLint viewport[4];
    
    gWindowW = w;
    gWindowH = h;
    
	// Call through to anyone else who needs to know about window sizing here.
    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = w;
    viewport[3] = h;
    VirtualEnvironmentHandleARViewUpdatedViewport(viewport);
}

static void print(const char *text, const float x, const float y)
{
    int i;
    size_t len;
    
    if (!text) return;
    len = strlen(text);
    glRasterPos2f(x, y);
    for (i = 0; i < len; i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, text[i]);
}

//
// This function is called when the window needs redrawing.
//
static void Display(void)
{
    char text[256];
	
	// Select correct buffer for this context.
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers for new frame.
    
	arglDispImage(gArglSettings);
				
    // Set up 3D mode.
	glMatrixMode(GL_PROJECTION);
#ifdef ARDOUBLE_IS_FLOAT
	glLoadMatrixf(cameraLens);
#else
	glLoadMatrixd(cameraLens);
#endif
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    glEnable(GL_DEPTH_TEST);

    // Set any initial per-frame GL state you require here.
    // --->
    
    // Lighting and geometry that moves with the camera should be added here.
    // (I.e. should be specified before camera pose transform.)
    // --->
    
    VirtualEnvironmentHandleARViewDrawPreCamera();
    
    if (cameraPoseValid) {
        
#ifdef ARDOUBLE_IS_FLOAT
        glMultMatrixf(cameraPose);
#else
        glMultMatrixd(cameraPose);
#endif
        
        // All lighting and geometry to be drawn in world coordinates goes here.
        // --->
        VirtualEnvironmentHandleARViewDrawPostCamera();
    }
    
    // Set up 2D mode.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (GLdouble)gWindowW, 0, (GLdouble)gWindowH, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Add your own 2D overlays here.
    // --->
    
    VirtualEnvironmentHandleARViewDrawOverlay();
    
    // Show some real-time info.
    glColor3ub(255, 255, 255);
    // FPS.
    snprintf(text, sizeof(text), "FPS %.1f", gFPS);
    print(text, 2.0f, (float)gWindowH - 12.0f);
	
	glutSwapBuffers();
}
