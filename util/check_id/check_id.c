/*
 *  check_id.c
 *  ARToolKit5
 *
 *  Allows visual verification of ARToolKit pattern ID's, including
 *  failure modes, ID numbers, and poses.
 *
 *  Press '?' while running for help on available key commands.
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
 *  Copyright 2010-2015 ARToolworks, Inc.
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
#endif
#include <stdlib.h>					// malloc(), free()
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
#include <AR/arMulti.h>
#include <ARUtil/time.h>

// ============================================================================
//	Constants
// ============================================================================

#define VIEW_DISTANCE_MIN		1.0			// Objects closer to the camera than this will not be displayed.
#define VIEW_DISTANCE_MAX		10000.0		// Objects further away from the camera than this will not be displayed.

typedef struct _cutoffPhaseColours {
    int cutoffPhase;
    GLubyte colour[3];
} cutoffPhaseColours_t ;

const cutoffPhaseColours_t cutoffPhaseColours[AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT] = {
    {AR_MARKER_INFO_CUTOFF_PHASE_NONE,                               {0xff, 0x0,  0x0 }},  // Red.
    {AR_MARKER_INFO_CUTOFF_PHASE_PATTERN_EXTRACTION,                 {0x95, 0xd6, 0xf6}},  // Light blue.
    {AR_MARKER_INFO_CUTOFF_PHASE_MATCH_GENERIC,                      {0x0,  0x0,  0xff}},  // Blue.
    {AR_MARKER_INFO_CUTOFF_PHASE_MATCH_CONTRAST,                     {0x99, 0x66, 0x33}},  // Brown.
    {AR_MARKER_INFO_CUTOFF_PHASE_MATCH_BARCODE_NOT_FOUND,            {0x7f, 0x0,  0x7f}},  // Purple.
    {AR_MARKER_INFO_CUTOFF_PHASE_MATCH_BARCODE_EDC_FAIL,             {0xff, 0x0,  0xff}},  // Magenta.
    {AR_MARKER_INFO_CUTOFF_PHASE_MATCH_CONFIDENCE,                   {0x0,  0xff, 0x0 }},  // Green.
    {AR_MARKER_INFO_CUTOFF_PHASE_POSE_ERROR,                         {0xff, 0x7f, 0x0 }},  // Orange.
    {AR_MARKER_INFO_CUTOFF_PHASE_POSE_ERROR_MULTI,                   {0xff, 0xff, 0x0 }},  // Yellow.
    {AR_MARKER_INFO_CUTOFF_PHASE_HEURISTIC_TROUBLESOME_MATRIX_CODES, {0xc6, 0xdc, 0x6a}},  // Khaki.
};

// ============================================================================
//	Global variables
// ============================================================================

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
static int          gPattSize = AR_PATT_SIZE1;
static int          gPattCountMax = AR_PATT_NUM_MAX;

// Transformation matrix retrieval.
static AR3DHandle	*gAR3DHandle = NULL;
static int          gRobustFlag = TRUE;
#define CHECK_ID_MULTIMARKERS_MAX 16
static int gMultiConfigCount = 0;
static ARMultiMarkerInfoT *gMultiConfigs[CHECK_ID_MULTIMARKERS_MAX] = {NULL};
static ARdouble gMultiErrs[CHECK_ID_MULTIMARKERS_MAX];

// Drawing.
static ARParamLT *gCparamLT = NULL;
static ARGL_CONTEXT_SETTINGS_REF gArglSettings = NULL;
static int gShowHelp = 1;
static int gShowMode = 1;
static GLint gViewport[4];
static int gDrawPatternSize = 0;
static ARUint8 ext_patt[AR_PATT_SIZE2_MAX*AR_PATT_SIZE2_MAX*3]; // Holds unwarped pattern extracted from image.


// ============================================================================
//	Function prototypes.
// ============================================================================

static void usage(char *com);
static int setupCamera(const char *cparam_name, char *vconf, ARParamLT **cparamLT_p, ARHandle **arhandle, AR3DHandle **ar3dhandle);
static int setupMarkers(const int patt_count, const char *patt_names[], ARMultiMarkerInfoT *multiConfigs[], ARHandle *arhandle, ARPattHandle **pattHandle_p);
static void cleanup(void);
static void Keyboard(unsigned char key, int x, int y);
static void Visibility(int visible);
static void Reshape(int w, int h);
static void Display(void);
static void print(const char *text, const float x, const float y, int calculateXFromRightEdge, int calculateYFromTopEdge);
static void drawBackground(const float width, const float height, const float x, const float y);
static void printHelpKeys();
static void printMode();

// ============================================================================
//	Functions
// ============================================================================

int main(int argc, char** argv)
{
	char glutGamemode[32];
    char *cpara = NULL;
	char cparaDefault[] = "../share/check_id/Data/camera_para.dat";
	char *vconf = NULL;
    int patt_names_count = 0;
    char *patt_names[CHECK_ID_MULTIMARKERS_MAX] = {NULL};
    ARdouble pattRatio = (ARdouble)AR_PATT_RATIO;
    AR_MATRIX_CODE_TYPE matrixCodeType = AR_MATRIX_CODE_TYPE_DEFAULT;
    int labelingMode = AR_DEFAULT_LABELING_MODE;
    int patternDetectionMode = AR_DEFAULT_PATTERN_DETECTION_MODE;
    int i, gotTwoPartOption;
    float tempF;
    int tempI;
    
	
    //
	// Library inits.
	//
    
	glutInit(&argc, argv);
    
    //
    // Startup options.
    //
    
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
            } else if (strcmp(argv[i], "--pattRatio") == 0) {
                i++;
                if (sscanf(argv[i], "%f", &tempF) == 1 && tempF > 0.0f && tempF < 1.0f) pattRatio = (ARdouble)tempF;
                else ARLOGe("Error: argument '%s' to --pattRatio invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--pattSize") == 0) {
                i++;
                if (sscanf(argv[i], "%d", &tempI) == 1 && tempI >= 16 && tempI <= AR_PATT_SIZE1_MAX) gPattSize = tempI;
                else ARLOGe("Error: argument '%s' to --pattSize invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--pattCountMax") == 0) {
                i++;
                if (sscanf(argv[i], "%d", &tempI) == 1 && tempI > 0) gPattCountMax = tempI;
                else ARLOGe("Error: argument '%s' to --pattSize invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--borderSize") == 0) {
                i++;
                if (sscanf(argv[i], "%f", &tempF) == 1 && tempF > 0.0f && tempF < 0.5f) pattRatio = (ARdouble)(1.0f - 2.0f*tempF);
                else ARLOGe("Error: argument '%s' to --borderSize invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--matrixCodeType") == 0) {
                i++;
                if (strcmp(argv[i], "AR_MATRIX_CODE_3x3") == 0) matrixCodeType = AR_MATRIX_CODE_3x3;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_3x3_HAMMING63") == 0) matrixCodeType = AR_MATRIX_CODE_3x3_HAMMING63;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_3x3_PARITY65") == 0) matrixCodeType = AR_MATRIX_CODE_3x3_PARITY65;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_4x4") == 0) matrixCodeType = AR_MATRIX_CODE_4x4;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_4x4_BCH_13_9_3") == 0) matrixCodeType = AR_MATRIX_CODE_4x4_BCH_13_9_3;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_4x4_BCH_13_5_5") == 0) matrixCodeType = AR_MATRIX_CODE_4x4_BCH_13_5_5;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_5x5") == 0) matrixCodeType = AR_MATRIX_CODE_5x5;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_6x6") == 0) matrixCodeType = AR_MATRIX_CODE_6x6;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_GLOBAL_ID") == 0) matrixCodeType = AR_MATRIX_CODE_GLOBAL_ID;
                else ARLOGe("Error: argument '%s' to --matrixCodeType invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--labelingMode") == 0) {
                i++;
                if (strcmp(argv[i], "AR_LABELING_BLACK_REGION") == 0) labelingMode = AR_LABELING_BLACK_REGION;
                else if (strcmp(argv[i], "AR_LABELING_WHITE_REGION") == 0) labelingMode = AR_LABELING_WHITE_REGION;
                else ARLOGe("Error: argument '%s' to --labelingMode invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--patternDetectionMode") == 0) {
                i++;
                if (strcmp(argv[i], "AR_TEMPLATE_MATCHING_COLOR") == 0) patternDetectionMode = AR_TEMPLATE_MATCHING_COLOR;
                else if (strcmp(argv[i], "AR_TEMPLATE_MATCHING_MONO") == 0) patternDetectionMode = AR_TEMPLATE_MATCHING_MONO;
                else if (strcmp(argv[i], "AR_MATRIX_CODE_DETECTION") == 0) patternDetectionMode = AR_MATRIX_CODE_DETECTION;
                else if (strcmp(argv[i], "AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX") == 0) patternDetectionMode = AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX;
                else if (strcmp(argv[i], "AR_TEMPLATE_MATCHING_MONO_AND_MATRIX") == 0) patternDetectionMode = AR_TEMPLATE_MATCHING_MONO_AND_MATRIX;
                else ARLOGe("Error: argument '%s' to --patternDetectionMode invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            }
        }
        if (!gotTwoPartOption) {
            // Look for single-part options.
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
                usage(argv[0]);
            } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
                ARLOG("%s version %s\n", argv[0], AR_HEADER_VERSION_STRING);
                exit(0);
            } else if( strncmp(argv[i], "-loglevel=", 10) == 0 ) {
                if (strcmp(&(argv[i][10]), "DEBUG") == 0) arLogLevel = AR_LOG_LEVEL_DEBUG;
                else if (strcmp(&(argv[i][10]), "INFO") == 0) arLogLevel = AR_LOG_LEVEL_INFO;
                else if (strcmp(&(argv[i][10]), "WARN") == 0) arLogLevel = AR_LOG_LEVEL_WARN;
                else if (strcmp(&(argv[i][10]), "ERROR") == 0) arLogLevel = AR_LOG_LEVEL_ERROR;
                else usage(argv[0]);
            } else if (strncmp(argv[i], "-border=", 8) == 0) {
                if (sscanf(&(argv[i][8]), "%f", &tempF) == 1 && tempF > 0.0f && tempF < 0.5f) pattRatio = (ARdouble)(1.0f - 2.0f*tempF);
                else ARLOGe("Error: argument '%s' to -border= invalid.\n", argv[i]);
            } else {
                if (patt_names_count < CHECK_ID_MULTIMARKERS_MAX) {
                    patt_names[patt_names_count] = argv[i];
                    patt_names_count++;
                }
            //} else {
            //    ARLOGe("Error: invalid command line argument '%s'.\n", argv[i]);
            //    usage(argv[0]);
            }
        }
        i++;
    }
    
	//
	// Video setup.
	//
    
    if (!cpara) cpara = cparaDefault;
	if (!setupCamera(cpara, vconf, &gCparamLT, &gARHandle, &gAR3DHandle)) {
		ARLOGe("main(): Unable to set up AR camera.\n");
		exit(-1);
	}
    
    //
    // AR init.
    //
    
    arSetPatternDetectionMode(gARHandle, patternDetectionMode);
    arSetLabelingMode(gARHandle, labelingMode);
    arSetPattRatio(gARHandle, pattRatio);
    arSetMatrixCodeType(gARHandle, matrixCodeType);
    
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
		glutInitWindowSize(gCparamLT->param.xsize, gCparamLT->param.ysize);
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
    if (!setupMarkers(patt_names_count, (const char **)patt_names, gMultiConfigs, gARHandle, &gARPattHandle)) {
        ARLOGe("main(): Unable to set up AR marker(s).\n");
        cleanup();
        exit(-1);
    }
    gMultiConfigCount = patt_names_count;
	
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
    ARLOG("Usage: %s [options] [Multimarker config. file [Multimarker config. file 2]]\n", com);
    ARLOG("Options:\n");
    ARLOG("  --vconf <video parameter for the camera>\n");
    ARLOG("  --cpara <camera parameter file for the camera>\n");
    ARLOG("  --pattRatio f: Specify the proportion of the marker width/height, occupied\n");
    ARLOG("             by the marker pattern. Range (0.0 - 1.0) (not inclusive).\n");
    ARLOG("             (I.e. 1.0 - 2*borderSize). Default value is 0.5.\n");
    ARLOG("  --pattSize n: Specify the number of rows and columns in the pattern space\n");
    ARLOG("             for template (pictorial) markers.\n");
    ARLOG("             Default value 16 (required for compatibility with ARToolKit prior\n");
    ARLOG("             to version 5.2). Range is [16, %d] (inclusive).\n", AR_PATT_SIZE1_MAX);
    ARLOG("  --pattCountMax n: Specify the maximum number of template (pictorial) markers\n");
    ARLOG("             that may be loaded for use in a single matching pass.\n");
    ARLOG("             Default value %d. Must be > 0.\n", AR_PATT_NUM_MAX);
    ARLOG("  --borderSize f: DEPRECATED specify the width of the pattern border, as a\n");
    ARLOG("             percentage of the marker width. Range (0.0 - 0.5) (not inclusive).\n");
    ARLOG("             (I.e. (1.0 - pattRatio)/2). Default value is 0.25.\n");
    ARLOG("  -border=f: Alternate syntax for --borderSize f.\n");
    ARLOG("  --matrixCodeType k: specify the type of matrix code used, where k is one of:\n");
    ARLOG("             AR_MATRIX_CODE_3x3 AR_MATRIX_CODE_3x3_HAMMING63\n");
    ARLOG("             AR_MATRIX_CODE_3x3_PARITY65 AR_MATRIX_CODE_4x4\n");
    ARLOG("             AR_MATRIX_CODE_4x4_BCH_13_9_3 AR_MATRIX_CODE_4x4_BCH_13_5_5\n");
    ARLOG("             AR_MATRIX_CODE_5x5 AR_MATRIX_CODE_6x6 AR_MATRIX_CODE_GLOBAL_ID\n");
    ARLOG("  --labelingMode AR_LABELING_BLACK_REGION|AR_LABELING_WHITE_REGION\n");
    ARLOG("  --patternDetectionMode k: specify the pattern detection mode, where k is one\n");
    ARLOG("             of: AR_TEMPLATE_MATCHING_COLOR AR_TEMPLATE_MATCHING_MONO\n");
    ARLOG("             AR_MATRIX_CODE_DETECTION AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX\n");
    ARLOG("             AR_TEMPLATE_MATCHING_MONO_AND_MATRIX\n");
    ARLOG("  -h -help --help: show this message\n");
    exit(0);
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

static int setupMarkers(const int patt_count, const char *patt_names[], ARMultiMarkerInfoT *multiConfigs[], ARHandle *arhandle, ARPattHandle **pattHandle_p)
{
    int i;
    
    if (!patt_count) {
        // Default behaviour is to default to matrix mode.
        *pattHandle_p = NULL;
        arSetPatternDetectionMode( arhandle, AR_MATRIX_CODE_DETECTION ); // If no markers specified, default to matrix mode.
    } else {
        // If marker configs have been specified, attempt to load them.
        
        int mode = -1, nextMode;
        
        // Need a pattern handle because the config file could specify matrix or template markers.
        if ((*pattHandle_p = arPattCreateHandle2(gPattSize, gPattCountMax)) == NULL) {
            ARLOGe("setupMarkers(): Error: arPattCreateHandle2.\n");
            return (FALSE);
        }
        
        for (i = 0; i < patt_count; i++) {
            
            if (!(multiConfigs[i] = arMultiReadConfigFile(patt_names[i], *pattHandle_p))) {
                ARLOGe("setupMarkers(): Error reading multimarker config file '%s'.\n", patt_names[i]);
                for (i--; i >= 0; i--) {
                    arMultiFreeConfig(multiConfigs[i]);
                }
                arPattDeleteHandle(*pattHandle_p);
                return (FALSE);
            }

            if (multiConfigs[i]->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE) {
                nextMode = AR_TEMPLATE_MATCHING_COLOR;
            } else if (multiConfigs[i]->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_MATRIX) {
                nextMode = AR_MATRIX_CODE_DETECTION;
            } else { // AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE_AND_MATRIX or mixed.
                nextMode = AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX;
            }
            
            if (mode == -1) {
                mode = nextMode;
            } else if (mode != nextMode) {
                mode = AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX;
            }
        }
        arSetPatternDetectionMode(arhandle, mode);
        
        arPattAttach(arhandle, *pattHandle_p);
    }
    
	return (TRUE);
}

static void cleanup(void)
{
    int i;
    
	arglCleanup(gArglSettings);
    gArglSettings = NULL;
    
    arPattDetach(gARHandle);
    for (i = 0; i < gMultiConfigCount; i++) {
        arMultiFreeConfig(gMultiConfigs[i]);
    }
	if (gARPattHandle) arPattDeleteHandle(gARPattHandle);
    
	arVideoCapStop();
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
            if (gShowHelp > 2) gShowHelp = 0;
			break;
        case 'r':
        case 'R':
            gRobustFlag = !gRobustFlag;
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
    int i, j;
    static int imageNumber = 0;
	static int ms_prev;
	int ms;
	float s_elapsed;
	AR2VideoBufferT *image;
    int pattDetectMode;
    AR_MATRIX_CODE_TYPE matrixCodeType;
    
	// Find out how long since mainLoop() last ran.
	ms = glutGet(GLUT_ELAPSED_TIME);
	s_elapsed = (float)(ms - ms_prev) * 0.001f;
	if (s_elapsed < 0.01f) return; // Don't update more often than 100 Hz.
	ms_prev = ms;
		
	// Grab a video frame.
    image = arVideoGetImage();
	if (image && image->fillFlag) {

        arglPixelBufferDataUpload(gArglSettings, image->buff);
        
        if (gARTImageSavePlease) {
            char imageNumberText[15];
            sprintf(imageNumberText, "image-%04d.jpg", imageNumber++);
            if (arVideoSaveImageJPEG(gARHandle->xsize, gARHandle->ysize, gARHandle->arPixelFormat, image->buff, imageNumberText, 75, 0) < 0) {
                ARLOGe("Error saving video image.\n");
            }
            gARTImageSavePlease = FALSE;
        }
		
		gCallCountMarkerDetect++; // Increment ARToolKit FPS counter.
		
		// Detect the markers in the video frame.
		if (arDetectMarker(gARHandle, image) < 0) {
			exit(-1);
		}
        
        // If  marker config files were specified, evaluate detected patterns against them now.
        for (i = 0; i < gMultiConfigCount; i++) {
            if (gRobustFlag) gMultiErrs[i] = arGetTransMatMultiSquareRobust(gAR3DHandle, arGetMarker(gARHandle), arGetMarkerNum(gARHandle), gMultiConfigs[i]);
            else gMultiErrs[i] = arGetTransMatMultiSquare(gAR3DHandle, arGetMarker(gARHandle), arGetMarkerNum(gARHandle), gMultiConfigs[i]);
            //if (gMultiConfigs[i]->prevF != 0) ARLOGe("Found multimarker set %d, err=%0.3f\n", i, gMultiErrs[i]);
        }
        
        
        // For matrix mode, draw the pattern image of the largest marker.
        arGetPatternDetectionMode(gARHandle, &pattDetectMode);
        arGetMatrixCodeType(gARHandle, &matrixCodeType);
        if (pattDetectMode == AR_MATRIX_CODE_DETECTION || pattDetectMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX || pattDetectMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX ) {
            
            int area = 0, biggestMarker = -1;
            
            for (j = 0; j < gARHandle->marker_num; j++) if (gARHandle->markerInfo[j].area > area) {
                area = gARHandle->markerInfo[j].area;
                biggestMarker = j;
            }
            if (area >= AR_AREA_MIN) {
                
                int imageProcMode;
                ARdouble pattRatio;
                ARdouble vertexUpright[4][2];
                
                // Reorder vertices based on dir.
                for (i = 0; i < 4; i++) {
                    int dir = gARHandle->markerInfo[biggestMarker].dir;
                    vertexUpright[i][0] = gARHandle->markerInfo[biggestMarker].vertex[(i + 4 - dir)%4][0];
                    vertexUpright[i][1] = gARHandle->markerInfo[biggestMarker].vertex[(i + 4 - dir)%4][1];
                }
                arGetImageProcMode(gARHandle, &imageProcMode);
                arGetPattRatio(gARHandle, &pattRatio);
                if (matrixCodeType == AR_MATRIX_CODE_GLOBAL_ID) {
                    gDrawPatternSize = 14;
                    arPattGetImage2(imageProcMode, AR_MATRIX_CODE_DETECTION, gDrawPatternSize, gDrawPatternSize * AR_PATT_SAMPLE_FACTOR2,
                                    image->buff, gARHandle->xsize, gARHandle->ysize, gARHandle->arPixelFormat, &gCparamLT->paramLTf, vertexUpright, (ARdouble)14/(ARdouble)(14 + 2), ext_patt);
                } else {
                    gDrawPatternSize = matrixCodeType & AR_MATRIX_CODE_TYPE_SIZE_MASK;
                    arPattGetImage2(imageProcMode, AR_MATRIX_CODE_DETECTION, gDrawPatternSize, gDrawPatternSize * AR_PATT_SAMPLE_FACTOR2,
                                    image->buff, gARHandle->xsize, gARHandle->ysize, gARHandle->arPixelFormat, &gCparamLT->paramLTf, vertexUpright, pattRatio, ext_patt);
                }
            } else {
                gDrawPatternSize = 0;
            }
        } else {
            gDrawPatternSize = 0;
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
    gViewport[0] = 0;
    gViewport[1] = 0;
    gViewport[2] = w;
    gViewport[3] = h;
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	
	// Call through to anyone else who needs to know about window sizing here.
}

static void drawAxes()
{
    GLfloat vertices[6][3] = {
        {0.0f, 0.0f, 0.0f}, {10.0f,  0.0f,  0.0f},
        {0.0f, 0.0f, 0.0f},  {0.0f, 10.0f,  0.0f},
        {0.0f, 0.0f, 0.0f},  {0.0f,  0.0f, 10.0f}
    };
    GLubyte colours[6][4] = {
        {255,0,0,255}, {255,0,0,255},
        {0,255,0,255}, {0,255,0,255},
        {0,0,255,255}, {0,0,255,255}
    };

    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colours);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glLineWidth(2.0f);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_LINES, 0, 6);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}

//
// This function is called when the window needs redrawing.
//
static void Display(void)
{
    ARdouble p[16];
    ARdouble m[16];
#ifdef ARDOUBLE_IS_FLOAT
    GLdouble p0[16];
    GLdouble m0[16];
#endif
    int i, j, k;
    GLfloat  w, bw, bh, vertices[6][2];
    GLubyte pixels[300];
    char text[256];
    GLdouble winX, winY, winZ;
    int showMErr[CHECK_ID_MULTIMARKERS_MAX];
    GLdouble MX[CHECK_ID_MULTIMARKERS_MAX];
    GLdouble MY[CHECK_ID_MULTIMARKERS_MAX];
    int pattDetectMode;
    AR_MATRIX_CODE_TYPE matrixCodeType;
   
    
    // Select correct buffer for this context.
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers for new frame.
	
	arglDispImage(gArglSettings);

    if (gMultiConfigCount) {
        arglCameraFrustumRH(&(gCparamLT->param), VIEW_DISTANCE_MIN, VIEW_DISTANCE_MAX, p);
        glMatrixMode(GL_PROJECTION);
#ifdef ARDOUBLE_IS_FLOAT
        glLoadMatrixf(p);
#else
        glLoadMatrixd(p);
#endif
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST);

        // If we have multi-configs, show their origin onscreen.
        for (k = 0; k < gMultiConfigCount; k++) {
            showMErr[k] = FALSE;
            if (gMultiConfigs[k]->prevF != 0) {
                arglCameraViewRH((const ARdouble (*)[4])gMultiConfigs[k]->trans, m, 1.0);
#ifdef ARDOUBLE_IS_FLOAT
                glLoadMatrixf(m);
#else
                glLoadMatrixd(m);
#endif
                drawAxes();
#ifdef ARDOUBLE_IS_FLOAT
                for (i = 0; i < 16; i++) m0[i] = (GLdouble)m[i];
                for (i = 0; i < 16; i++) p0[i] = (GLdouble)p[i];
                if (gluProject(0, 0, 0, m0, p0, gViewport, &winX, &winY, &winZ) == GL_TRUE)
#else
                if (gluProject(0, 0, 0, m, p, gViewport, &winX, &winY, &winZ) == GL_TRUE)
#endif
                {
                    showMErr[k] = TRUE;
                    MX[k] = winX; MY[k] = winY;
                }
            }
            
        } // for k
    }
    
	// Any 2D overlays go here.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (GLdouble)windowWidth, 0, (GLdouble)windowHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    arGetPatternDetectionMode(gARHandle, &pattDetectMode);
    arGetMatrixCodeType(gARHandle, &matrixCodeType);

    // For all markers, draw onscreen position.
    // Colour based on cutoffPhase.
    glLoadIdentity();
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glLineWidth(2.0f);
    for (j = 0; j < gARHandle->marker_num; j++) {
        glColor3ubv(cutoffPhaseColours[gARHandle->markerInfo[j].cutoffPhase].colour);
        for (i = 0; i < 5; i++) {
            int dir = gARHandle->markerInfo[j].dir;
            vertices[i][0] = (float)gARHandle->markerInfo[j].vertex[(i + 4 - dir)%4][0] * (float)windowWidth / (float)gARHandle->xsize;
            vertices[i][1] = ((float)gARHandle->ysize - (float)gARHandle->markerInfo[j].vertex[(i + 4 - dir)%4][1]) * (float)windowHeight / (float)gARHandle->ysize;
        }
        vertices[i][0] = (float)gARHandle->markerInfo[j].pos[0] * (float)windowWidth / (float)gARHandle->xsize;
        vertices[i][1] = ((float)gARHandle->ysize - (float)gARHandle->markerInfo[j].pos[1]) * (float)windowHeight / (float)gARHandle->ysize;
        glDrawArrays(GL_LINE_STRIP, 0, 6);
        // For markers that have been identified, draw the ID number.
        if (gARHandle->markerInfo[j].id >= 0) {
            glColor3ub(255, 0, 0);
            if (matrixCodeType == AR_MATRIX_CODE_GLOBAL_ID && (pattDetectMode == AR_MATRIX_CODE_DETECTION || pattDetectMode == AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX || pattDetectMode == AR_TEMPLATE_MATCHING_MONO_AND_MATRIX)) snprintf(text, sizeof(text), "%llu (err=%d)", gARHandle->markerInfo[j].globalID, gARHandle->markerInfo[j].errorCorrected);
            else snprintf(text, sizeof(text), "%d", gARHandle->markerInfo[j].id);
            print(text, (float)gARHandle->markerInfo[j].pos[0] * (float)windowWidth / (float)gARHandle->xsize, ((float)gARHandle->ysize - (float)gARHandle->markerInfo[j].pos[1]) * (float)windowHeight / (float)gARHandle->ysize, 0, 0);
        }
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    
    // For matrix mode, draw the pattern image of the largest marker.
    if (gDrawPatternSize) {
        int zoom = 4;
        glRasterPos2f((float)(windowWidth - gDrawPatternSize*zoom) - 4.0f, (float)(gDrawPatternSize*zoom) + 4.0f);
        glPixelZoom((float)zoom, (float)-zoom);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glDrawPixels(gDrawPatternSize, gDrawPatternSize, GL_LUMINANCE, GL_UNSIGNED_BYTE, ext_patt);
        glPixelZoom(1.0f, 1.0f);
    }

    
    // Draw error value for multimarker pose.
    for (k = 0; k < gMultiConfigCount; k++) {
        if (showMErr[k]) {
            snprintf(text, sizeof(text), "err=%0.3f", gMultiErrs[k]);
            print(text, MX[k], MY[k], 0, 0);
        }
    }
    
    //
    // Draw help text and mode.
    //
    glLoadIdentity();
    if (gShowMode) {
        printMode();
    }
    if (gShowHelp) {
        if (gShowHelp == 1) {
            printHelpKeys();
        } else if (gShowHelp == 2) {
            bw = 0.0f;
            for (i = 0; i < AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT; i++) {
                w = (float)glutBitmapLength(GLUT_BITMAP_HELVETICA_10, (unsigned char *)arMarkerInfoCutoffPhaseDescriptions[cutoffPhaseColours[i].cutoffPhase]);
                if (w > bw) bw = w;
            }
            bw += 12.0f; // Space for color block.
            bh = AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT * 10.0f /* character height */+ (AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT - 1) * 2.0f /* line spacing */;
            drawBackground(bw, bh, 2.0f, 2.0f);
            
            // Draw the colour block and text, line by line.
            for (i = 0; i < AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT; i++) {
                for (j = 0; j < 300; j += 3) {
                    pixels[j    ] = cutoffPhaseColours[i].colour[0];
                    pixels[j + 1] = cutoffPhaseColours[i].colour[1];
                    pixels[j + 2] = cutoffPhaseColours[i].colour[2];
                }
                glRasterPos2f(2.0f, (AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT - 1 - i) * 12.0f + 2.0f);
                glPixelZoom(1.0f, 1.0f);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glDrawPixels(10, 10, GL_RGB, GL_UNSIGNED_BYTE, pixels);
                print(arMarkerInfoCutoffPhaseDescriptions[cutoffPhaseColours[i].cutoffPhase], 14.0f, (AR_MARKER_INFO_CUTOFF_PHASE_DESCRIPTION_COUNT - 1 - i) * 12.0f + 2.0f, 0, 0);
            }
        }
    }
    
	glutSwapBuffers();
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
        " ? or /        Show/hide this help / marker cutoff phase key.",
        " q or [esc]    Quit program.",
        " d             Activate / deactivate debug mode.",
        " m             Toggle display of mode info.",
        " a             Toggle between available threshold modes.",
        " - and +       Switch to manual threshold mode, and adjust threshhold up/down by 5.",
        " x             Change image processing mode.",
        " c             Calulcate frame rate.",
        " r             Toggle robust multi-marker mode on/off.",
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
    int len, thresh, line, mode, xsize, ysize, textPatternCount;
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
    snprintf(text, sizeof(text), "Border: %0.2f%%", tempF*100.0);
    arGetPatternDetectionMode(gARHandle, &mode);
    textPatternCount = 0;
    switch (mode) {
        case AR_TEMPLATE_MATCHING_COLOR: text_p = "Colour template (pattern)"; break;
        case AR_TEMPLATE_MATCHING_MONO: text_p = "Mono template (pattern)"; break;
        case AR_MATRIX_CODE_DETECTION: text_p = "Matrix (barcode)"; textPatternCount = -1; break;
        case AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX: text_p = "Colour template + Matrix (2 pass, pattern + barcode)"; break;
        case AR_TEMPLATE_MATCHING_MONO_AND_MATRIX: text_p = "Mono template + Matrix (2 pass, pattern + barcode "; break;
        default: text_p = "UNKNOWN"; textPatternCount = -1; break;
    }
    if (textPatternCount != -1) textPatternCount = gARPattHandle->patt_num;
    len = (int)strlen(text);
    if (textPatternCount != -1) snprintf(text + len, sizeof(text) - len, ", Pattern detection mode: %s, %d patterns loaded", text_p, textPatternCount);
    else snprintf(text + len, sizeof(text) - len, ", Pattern detection mode: %s", text_p);
    print(text, 2.0f,  (line - 1)*12.0f + 2.0f, 0, 1);
    line++;
    
    // Robust mode.
    if (gMultiConfigCount) {
        snprintf(text, sizeof(text), "Robust multi-marker pose estimation %s", (gRobustFlag ? "ON" : "OFF"));
        if (gRobustFlag) {
            icpGetInlierProbability(gAR3DHandle->icpHandle, &tempF);
            len = (int)strlen(text);
            snprintf(text + len, sizeof(text) - len, ", inliner prob. %0.1f%%", tempF*100.0f);
        }
        print(text, 2.0f,  (line - 1)*12.0f + 2.0f, 0, 1);
        line++;
    }
    
    // Window size.
    snprintf(text, sizeof(text), "Drawing into %dx%d window", windowWidth, windowHeight);
    print(text, 2.0f,  (line - 1)*12.0f + 2.0f, 0, 1);
    line++;
    
}
