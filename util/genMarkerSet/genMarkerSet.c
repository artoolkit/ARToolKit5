/*
 *  genMarkerSet.c
 *  ARToolKit5
 *
 *  Identifies markers in texture image and generates marker set files.
 *
 *  Run with "--help" parameter to see usage.
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

#ifdef _WIN32
#  include <windows.h>
#  define snprintf _snprintf
#endif
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#  define MAXPATHLEN MAX_PATH
#else
#  include <sys/param.h> // MAXPATHLEN
#endif
#ifndef __APPLE__
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif
#include <AR/ar.h>
#include <AR/gsub.h>
#include <AR2/imageSet.h>
#include <AR2/marker.h>
#include <AR2/util.h>
#include <AR2/imageFormat.h>
#include <AR/video.h>

#define		XWIN_MAX	1000
#define		YWIN_MAX	800
#define		THRESH		100

enum {
    E_NO_ERROR = 0,
    E_BAD_PARAMETER = 64,
    E_INPUT_DATA_ERROR = 65,
    E_USER_INPUT_CANCELLED = 66,
    E_BACKGROUND_OPERATION_UNSUPPORTED = 69,
    E_DATA_PROCESSING_ERROR = 70,
    E_UNABLE_TO_DETACH_FROM_CONTROLLING_TERMINAL = 71,
    E_GENERIC_ERROR = 255
};


static AR2ImageSetT    *imageSet = NULL;
static AR2JpegImageT   *jpegImage = NULL;
static AR2VideoBufferT  buff = {0};
static int              xsize = 0, ysize = 0; // Input image size.
static int              windowWidth = XWIN_MAX;
static int              windowHeight = YWIN_MAX;
static ARGViewportHandle *vp = NULL;
static AR_PIXEL_FORMAT  pixFormat = AR_PIXEL_FORMAT_INVALID;
static float            dpi = -1.0f;
static char            *inputFilePath = NULL;
static ARVideoLumaInfo *lumaInfo = NULL;
static ARdouble         pattRatio = (ARdouble)(AR_PATT_RATIO);
static int              gPattSize = AR_PATT_SIZE1;
static ARParam          cparam;
static ARParamLT       *cparamLT = NULL;
static ARHandle        *gARHandle = NULL;
static float           *gMarkerWidths = NULL;
static int              detectedMarkerNum = 0;
static int              gCapturing = 0;
//static int              gCapturingMarker = -1;
static int              gShowHelp = 1;
static int              gShowMode = 1;

static char             exitcode = -1;
#define EXIT(c) {exitcode=c;exit(c);}

static int  init( int argc, char *argv[] );
static int detect(float **markerWidths_p);
static void cleanup(void);
static void usage( char *com );
static void keyEvent( unsigned char key, int x, int y);
static void mouseEvent(int button, int state, int x, int y);
static void dispFunc( void );
static void check_square( ARMarkerInfo *markerInfo, int markerNum, float *markerWidths );
static void pixel2mm( float px, float py, float *mx, float *my );
//static void mm2pixel( float mx, float my, float *px, float *py );
static void print(const char *text, const float x, const float y, int calculateXFromRightEdge, int calculateYFromTopEdge);
static void drawBackground(const float width, const float height, const float x, const float y);
static void printHelpKeys(void);
static void printMode(void);

int main( int argc, char *argv[] )
{
    glutInit(&argc, argv);
    init( argc, argv );
    argSetDispFunc( dispFunc, 0 );
    argSetKeyFunc( keyEvent );
    //argSetMouseFunc( mouseEvent );
    argMainLoop();
    return (0);
}


static int init( int argc, char *argv[] )
{
    ARGViewport                         viewport;
    char                               *ext = NULL;
    float                               xzoom, yzoom, zoom;
    int                                 i;
    int                                 gotTwoPartOption;
    float                               tempF;
    int                                 tempI;
        
    i = 1; // argv[0] is name of app, so start at 1.
    while (i < argc) {
        gotTwoPartOption = FALSE;
        // Look for two-part options first.
        if ((i + 1) < argc) {
            if (strcmp(argv[i], "--pattRatio") == 0) {
                i++;
                if (sscanf(argv[i], "%f", &tempF) == 1 && tempF > 0.0f && tempF < 1.0f) pattRatio = (ARdouble)tempF;
                else ARLOGe("Error: argument '%s' to --pattRatio invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--pattSize") == 0) {
                i++;
                if (sscanf(argv[i], "%d", &tempI) == 1 && tempI >= 16 && tempI <= AR_PATT_SIZE1_MAX) gPattSize = tempI;
                else ARLOGe("Error: argument '%s' to --pattSize invalid.\n", argv[i]);
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--borderSize") == 0) {
                i++;
                if (sscanf(argv[i], "%f", &tempF) == 1 && tempF > 0.0f && tempF < 0.5f) pattRatio = (ARdouble)(1.0f - 2.0f*tempF);
                else ARLOGe("Error: argument '%s' to --borderSize invalid.\n", argv[i]);
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
            } else if (strncmp(argv[i], "-dpi=", 5) == 0) {
                if (sscanf(&(argv[i][5]), "%f", &tempF) == 1 && tempF > 0.0f) dpi = tempF;
                else ARLOGe("Error: argument '%s' to -dpi= invalid.\n", argv[i]);
            } else {
                if (!inputFilePath) inputFilePath = strdup(argv[i]);
                else usage(argv[0]);
            }
        }
        i++;
    }
    
    if (!inputFilePath) usage(argv[0]);

    ext = arUtilGetFileExtensionFromPath(inputFilePath, 1);
    if (!ext) {
        ARLOGe("Error: unable to determine extension of file '%s'. Exiting.\n", inputFilePath);
        EXIT(E_INPUT_DATA_ERROR);
    }
    
    if (strcmp(ext, "iset") == 0) {
        
        int targetScale;
		char *filenameISet;
        
        ARLOGi("Reading ImageSet...\n");
        filenameISet = strdup(inputFilePath);
        ar2UtilRemoveExt(filenameISet);
        imageSet = ar2ReadImageSet(filenameISet);
        if( imageSet == NULL ) {
            ARLOGe("Error: unable to read ImageSet from file '%s.iset'\n", filenameISet);
            free(ext);
            free(filenameISet);
            EXIT(E_INPUT_DATA_ERROR);
        }
        free(filenameISet);
        ARLOGi("   Done.\n");
       
        targetScale = 0;
        for( i = 1; i < imageSet->num; i++ ) {
            if( imageSet->scale[i]->dpi > imageSet->scale[targetScale]->dpi ) targetScale = i;
        }
        pixFormat = AR_PIXEL_FORMAT_MONO;
        
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
        buff.buff = imageSet->scale[targetScale]->imgBWBlur[1];
        buff.fillFlag = 1;
#else
        buff.buff = imageSet->scale[targetScale]->imgBW;
        buff.fillFlag = 1;
#endif
        xsize = imageSet->scale[targetScale]->xsize;
        ysize = imageSet->scale[targetScale]->ysize;
        dpi   = imageSet->scale[targetScale]->dpi;
        ARLOGi("iset image '%s' is %dx%d@%f dpi.\n", inputFilePath, xsize, ysize, dpi);
        
    } else if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0 || strcmp(ext, "jpe") == 0) {
        char buf[256];
        char buf1[MAXPATHLEN], buf2[MAXPATHLEN];
        
        ARLOGi("Reading JPEG file...\n");
        ar2UtilDivideExt(inputFilePath, buf1, buf2);
        jpegImage = ar2ReadJpegImage(buf1, buf2);
        if (!jpegImage) {
            ARLOGe("Error: unable to read JPEG image from file '%s'. Exiting.\n", inputFilePath);
            free(ext);
            EXIT(E_INPUT_DATA_ERROR);
        }
        ARLOGi("   Done.\n");
        
        ARLOGi("JPEG image '%s' is %dx%d.\n", inputFilePath, jpegImage->xsize, jpegImage->ysize);
        buff.buff = jpegImage->image;
        buff.fillFlag = 1;
        if (jpegImage->nc == 1) pixFormat = AR_PIXEL_FORMAT_MONO;
        else if (jpegImage->nc == 3) pixFormat = AR_PIXEL_FORMAT_RGB;
        else {
            ARLOGe("Error: 2 byte/pixel JPEG files not currently supported. Exiting.\n");
            ar2FreeJpegImage(&jpegImage);
            free(ext);
            EXIT(E_INPUT_DATA_ERROR);
        }
        xsize = jpegImage->xsize;
        ysize = jpegImage->ysize;
        if (dpi == -1.0f) {
            if( jpegImage->dpi == 0.0f ) {
                for (;;) {
                    printf("JPEG image '%s' does not contain embedded resolution data, and no resolution specified on command-line.\nEnter resolution to use (in decimal DPI): ", inputFilePath);
                    if( fgets( buf, sizeof(buf), stdin ) == NULL ) {
                        ar2FreeJpegImage(&jpegImage);
                        free(ext);
                        EXIT(E_USER_INPUT_CANCELLED);
                    }
                    if( sscanf(buf, "%f", &(jpegImage->dpi)) == 1 ) break;
                }
            }
            dpi   = jpegImage->dpi;
        }

        // Create luma-only buffer.
        if (pixFormat == AR_PIXEL_FORMAT_MONO) {
            buff.buffLuma = buff.buff;
        } else {
            lumaInfo = arVideoLumaInit(xsize, ysize, pixFormat);
            if (!lumaInfo) {
                ARLOGe("Error: unable to initialise luma conversion.\n");
                ar2FreeJpegImage(&jpegImage);
                free(ext);
                EXIT(E_INPUT_DATA_ERROR);
            }
            buff.buffLuma = arVideoLuma(lumaInfo, buff.buff);
        }

    } else {
        ARLOGe("Error: file '%s' has extension '%s', which is not supported for reading. Exiting.\n", inputFilePath, ext);
        free(ext);
        EXIT(E_INPUT_DATA_ERROR);
    }
    free(ext);

    xzoom = yzoom = 1.0;
    while( xsize > windowWidth*xzoom ) xzoom += 1.0;
    while( ysize > windowHeight*yzoom ) yzoom += 1.0;
    if( xzoom > yzoom ) zoom = 1.0/xzoom;
    else                zoom = 1.0/yzoom;
    windowWidth = xsize * zoom;
    windowHeight = ysize * zoom;
    ARLOG("Size = (%d,%d) Zoom = %f\n", xsize, ysize, zoom);

    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = windowWidth;
    viewport.ysize = windowHeight;
    vp = argCreateViewport( &viewport );
    argViewportSetImageSize( vp, xsize, ysize );
    argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_GL_DRAW_PIXELS );
    argViewportSetDispMode( vp, AR_GL_DISP_MODE_FIT_TO_VIEWPORT );
    argViewportSetDistortionMode( vp, AR_GL_DISTORTION_COMPENSATE_DISABLE );

    arParamClear( &cparam, xsize, ysize, AR_DIST_FUNCTION_VERSION_DEFAULT );
    cparamLT = arParamLTCreate( &cparam, AR_PARAM_LT_DEFAULT_OFFSET );
    gARHandle = arCreateHandle( cparamLT );
    arSetDebugMode( gARHandle, AR_DEBUG_ENABLE );
    arSetLabelingMode( gARHandle, AR_LABELING_BLACK_REGION );
    arSetLabelingThresh( gARHandle, THRESH );
    arSetImageProcMode( gARHandle, AR_IMAGE_PROC_FRAME_IMAGE );
    arSetMarkerExtractionMode( gARHandle, AR_NOUSE_TRACKING_HISTORY );
    arSetPixelFormat( gARHandle, pixFormat );

    detectedMarkerNum = detect(&gMarkerWidths);

    return 0;
}

static int detect(float **markerWidths_p)
{
    int i, j;
    
    if (!markerWidths_p) return 0;
    
    arDetectMarker(gARHandle, &buff);
    ARLOG("Pass 1: detected %d markers.\n", gARHandle->marker_num);
    
    if (*markerWidths_p) free(*markerWidths_p);
    arMalloc( *markerWidths_p, float, gARHandle->marker_num );
    check_square( gARHandle->markerInfo, gARHandle->marker_num, *markerWidths_p );
    for( i = j = 0; i < gARHandle->marker_num; i++ ) {
        if ((*markerWidths_p)[i] > 0.0 ) j++;
    }
    ARLOG("Pass 2: %d detected markers are square.\n", j);
    
    return j;
}

static void cleanup(void)
{
    if (gMarkerWidths) {
        free(gMarkerWidths);
        gMarkerWidths = NULL;
    }
    argCleanup();
    if (lumaInfo) {
        arVideoLumaFinal(&lumaInfo);
        lumaInfo = NULL;
    }
    ar2FreeImageSet(&imageSet);
    ar2FreeJpegImage(&jpegImage);
    free(inputFilePath);
}

static void usage( char *com )
{
    ARLOG("Usage: %s [options] <filename>\n\n", com);
    ARLOG("Where <filename> is path to a JPEG or iset file.\n\n");
    ARLOG("Options:\n");
    ARLOG("  --pattRatio f: Specify the proportion of the marker width/height, occupied\n");
    ARLOG("             by the marker pattern. Range (0.0 - 1.0) (not inclusive).\n");
    ARLOG("             (I.e. 1.0 - 2*borderSize). Default value is 0.5.\n");
    ARLOG("  --borderSize f: DEPRECATED specify the width of the pattern border, as a\n");
    ARLOG("             percentage of the marker width. Range (0.0 - 0.5) (not inclusive).\n");
    ARLOG("             (I.e. (1.0 - pattRatio)/2). Default value is 0.25.\n");
    ARLOG("  -border=f: Alternate syntax for --borderSize f.\n");
    ARLOG("  --pattSize n: Specify the number of rows and columns in the pattern space\n");
    ARLOG("             for template (pictorial) markers.\n");
    ARLOG("             Default value 16 (required for compatibility with ARToolKit prior\n");
    ARLOG("             to version 5.2). Range is [16, %d] (inclusive).\n", AR_PATT_SIZE1_MAX);
    ARLOG("  -dpi=f:    Override embedded JPEG DPI value.\n");
    ARLOG("  -h -help --help: show this message\n");
    exit(0);
}

static void   keyEvent( unsigned char key, int x, int y)
{
    int mode, threshChange = 0, redetect = 0, redraw = 0;
    AR_LABELING_THRESH_MODE modea;
    
    switch (key) {
        case 0x1B:						// Quit.
        case 'Q':
        case 'q':
            cleanup();
            exit(0);
            break;
        case ' ':
            gCapturing = 1;
            redraw = 1;
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
            redetect = redraw = 1;
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
            redetect = redraw = 1;
            break;
        case 'b':
        case 'B':
            arGetLabelingMode(gARHandle, &mode);
            if (mode == AR_LABELING_BLACK_REGION) mode = AR_LABELING_WHITE_REGION;
            else mode = AR_LABELING_BLACK_REGION;
            arSetLabelingMode(gARHandle, mode);
            redetect = redraw = 1;
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
            redraw = 1;
            break;
        case '?':
        case '/':
            gShowHelp++;
            if (gShowHelp > 1) gShowHelp = 0;
            redraw = 1;
            break;
        case 'm':
        case 'M':
            gShowMode = !gShowMode;
            redraw = 1;
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
        ARLOG("thresh = %d\n", threshhold);
        redetect = redraw = 1;
    }

    if (redetect) {
        detectedMarkerNum = detect(&gMarkerWidths);
    }
    if (redraw) {
        glutPostRedisplay();
    }
}

static void mouseEvent(int button, int state, int x, int y)
{
    if( button == GLUT_RIGHT_BUTTON  && state == GLUT_DOWN ) {
        cleanup();
        exit(0);
    }
    if( button == GLUT_LEFT_BUTTON  && state == GLUT_DOWN ) {
        // Capture the click, project it to a marker, and save it.
    }
}


static void dispFunc( void )
{
    int     ret = 0, mode;
    char    path[MAXPATHLEN] = "";
    size_t  pathLen = 0L;
    char   *basename = NULL;
    const char markerExt[] = "mrk";
    const char patternExt[] = "pat";
    FILE    *fp;
    char    buf[256];
    int     i, j;
    int	    ii, jj;
    int     x, y;
    int     co, *flag;
    float   vec[2][2], center[2], length;
    float   trans1[3][4];
    float   mx1, my1, mx2, my2;
    //static int r = 0;

    glClear(GL_COLOR_BUFFER_BIT);

    if (!gCapturing) {
        arGetDebugMode(gARHandle, &mode);
        if (mode == AR_DEBUG_ENABLE) {
            argViewportSetPixFormat(vp, AR_PIXEL_FORMAT_MONO); // Drawing the debug image.
            argDrawMode2D(vp);
            arGetImageProcMode(gARHandle, &mode);
            if (mode == AR_IMAGE_PROC_FRAME_IMAGE) argDrawImage(gARHandle->labelInfo.bwImage);
            else argDrawImageHalf(gARHandle->labelInfo.bwImage);
        } else {
            argViewportSetPixFormat(vp, pixFormat); // Drawing the input image.
            argDrawMode2D(vp);
            argDrawImage(buff.buff);
        }
        
        // Draw detected markers.
        glLineWidth(1.0f);
        glColor3f(0.0f, 1.0f, 0.0f);
        for (i = j = 0; i < detectedMarkerNum; i++) {
            while (gMarkerWidths[j] <= 0.0f) j++;
            argDrawSquareByIdealPos(gARHandle->markerInfo[j].vertex);
            argDrawLineByIdealPos(gARHandle->markerInfo[j].vertex[2][0], gARHandle->markerInfo[j].vertex[2][1],  gARHandle->markerInfo[j].pos[0], gARHandle->markerInfo[j].pos[1]);
            j++;
        }

        //
        // Draw help text and mode.
        //
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        if (gShowMode) {
            printMode();
        }
        if (gShowHelp) {
            if (gShowHelp == 1) {
                printHelpKeys();
            }
        }
        
        argSwapBuffers();
        return;
    }

    basename = arUtilGetFileBasenameFromPath(inputFilePath, 0);
    
    arUtilGetDirectoryNameFromPath(path, inputFilePath, sizeof(path), 1);
    pathLen = strlen(path);
    snprintf(path + pathLen, sizeof(path) - pathLen, "%s.%s", basename, markerExt);
    if (!(fp = fopen(path, "w"))) {
        ARLOGe("Error opening output marker file '%s' for writing.\n", path);
        ARLOGperror(NULL);
        ret = -1;
        goto done0;
    }

    co = 0;
    arMalloc( flag, int, detectedMarkerNum );
    for(ii = jj = 0; ii < detectedMarkerNum; ii++, jj++) {

        arGetDebugMode(gARHandle, &mode);
        if (mode == AR_DEBUG_ENABLE) {
            argViewportSetPixFormat(vp, AR_PIXEL_FORMAT_MONO); // Drawing the debug image.
            argDrawMode2D(vp);
            arGetImageProcMode(gARHandle, &mode);
            if (mode == AR_IMAGE_PROC_FRAME_IMAGE) argDrawImage(gARHandle->labelInfo.bwImage);
            else argDrawImageHalf(gARHandle->labelInfo.bwImage);
        } else {
            argViewportSetPixFormat(vp, pixFormat); // Drawing the input image.
            argDrawMode2D(vp);
            argDrawImage(buff.buff);
        }

        // Draw detected markers.
        for (i = j = 0; i < detectedMarkerNum; i++) {
            while (gMarkerWidths[j] <= 0.0f) j++;
            glLineWidth(1.0f);
            glColor3f(0.0f, 1.0f, 0.0f);
            argDrawSquareByIdealPos(gARHandle->markerInfo[j].vertex);
            argDrawLineByIdealPos(gARHandle->markerInfo[j].vertex[2][0], gARHandle->markerInfo[j].vertex[2][1], gARHandle->markerInfo[j].pos[0], gARHandle->markerInfo[j].pos[1]);
            j++;
        }
        
        // Draw marker of interest.
        while (gMarkerWidths[jj] <= 0.0f) jj++;
        glLineWidth(2.0f);
        glColor3f(1.0f, 0.0f, 0.0f);
        argDrawSquareByIdealPos(gARHandle->markerInfo[jj].vertex);
        argDrawLineByIdealPos(gARHandle->markerInfo[jj].vertex[2][0], gARHandle->markerInfo[jj].vertex[2][1], gARHandle->markerInfo[jj].pos[0], gARHandle->markerInfo[jj].pos[1]);
        
        argSwapBuffers();

        flag[ii] = 0;
        for(;;) {
            printf("Save this marker? (y or n): ");
            if (!fgets(buf, sizeof(buf), stdin)) {
                ret = -1;
                goto done;
            }
            if (buf[0] == 'y' || buf[0] == 'n') break;
        }
        if( buf[0] == 'n' ) continue;
        flag[ii] = 1;
        co++;

        arUtilGetDirectoryNameFromPath(path, inputFilePath, sizeof(path), 1);
        pathLen = strlen(path);
        snprintf(path + pathLen, sizeof(path) - pathLen, "%s-%02d.%s", basename, co, patternExt);
        if (arPattSave(buff.buff, xsize, ysize, pixFormat, &(cparamLT->paramLTf), AR_IMAGE_PROC_FRAME_IMAGE, &(gARHandle->markerInfo[jj]), pattRatio, gPattSize, path) < 0) {
            ARLOGe("Error saving pattern file '%s'.\n", path);
            ret = -1;
            goto done;
        } else {
            ARLOG("Saved pattern file '%s'.\n", path);
        }
    }

    // Write the marker file.
    fprintf(fp, "%d\n\n", co);
    for( i = ii = j = 0; i < detectedMarkerNum; i++, j++ ) {
        while( gMarkerWidths[j] <= 0.0 ) j++;
        if( flag[i] == 0 ) continue;

        snprintf(path, sizeof(path), "%s-%02d.%s", basename, ii+1, patternExt);
        fprintf(fp, "%s\n", path);
        fprintf(fp, "%f\n", gMarkerWidths[j]);

        // Print vertices in pixel coords.
        // Origin at UL of image.
        ARLOG("-- %s --\n", path);
        ARLOG("Upper-left:  {%f, %f}\n", gARHandle->markerInfo[j].vertex[2][0], gARHandle->markerInfo[j].vertex[2][1]);
        ARLOG("Upper-right: {%f, %f}\n", gARHandle->markerInfo[j].vertex[3][0], gARHandle->markerInfo[j].vertex[3][1]);
        ARLOG("Lower-right: {%f, %f}\n", gARHandle->markerInfo[j].vertex[0][0], gARHandle->markerInfo[j].vertex[0][1]);
        ARLOG("Lower-left:  {%f, %f}\n", gARHandle->markerInfo[j].vertex[1][0], gARHandle->markerInfo[j].vertex[1][1]);

        // UR (for axis-aligned upright marker) to m1, UL (for axis-aligned upright marker) to m2
        // Origin at LL of image.
        // vec[0] is unit vector from UL to UR (for axis-aligned upright marker).
        pixel2mm( gARHandle->markerInfo[j].vertex[3][0], gARHandle->markerInfo[j].vertex[3][1], &mx1, &my1 );
        pixel2mm( gARHandle->markerInfo[j].vertex[2][0], gARHandle->markerInfo[j].vertex[2][1], &mx2, &my2 );
        vec[0][0] = mx1 - mx2;
        vec[0][1] = my1 - my2;
        length = sqrt( vec[0][0]*vec[0][0] + vec[0][1]*vec[0][1] );
        vec[0][0] /= length;
        vec[0][1] /= length;

        // LL (for axis-aligned upright marker) to m1, UL (for axis-aligned upright marker) to m2
        // Origin at LL of image.
        // vec[1] is unit vector from LL to UL (for axis-aligned upright marker).
        pixel2mm( gARHandle->markerInfo[j].vertex[1][0], gARHandle->markerInfo[j].vertex[1][1], &mx1, &my1 );
        pixel2mm( gARHandle->markerInfo[j].vertex[2][0], gARHandle->markerInfo[j].vertex[2][1], &mx2, &my2 );
        vec[1][0] = mx2 - mx1;
        vec[1][1] = my2 - my1;
        length = sqrt( vec[1][0]*vec[1][0] + vec[1][1]*vec[1][1] );
        vec[1][0] /= length;
        vec[1][1] /= length;

        mx1 = (gARHandle->markerInfo[j].vertex[0][0] + gARHandle->markerInfo[j].vertex[1][0]
             + gARHandle->markerInfo[j].vertex[2][0] + gARHandle->markerInfo[j].vertex[3][0]) / 4;
        my1 = (gARHandle->markerInfo[j].vertex[0][1] + gARHandle->markerInfo[j].vertex[1][1]
             + gARHandle->markerInfo[j].vertex[2][1] + gARHandle->markerInfo[j].vertex[3][1]) / 4;
        pixel2mm( mx1, my1, &mx2, &my2 );
        center[0] = mx2;
        center[1] = my2;

        trans1[0][0] = vec[0][0];
        trans1[1][0] = vec[0][1];
        trans1[2][0] = 0.0;
        trans1[0][1] = vec[1][0];
        trans1[1][1] = vec[1][1];
        trans1[2][1] = 0.0;
        trans1[0][2] = 0.0;
        trans1[1][2] = 0.0;
        trans1[2][2] = 1.0;
        trans1[0][3] = center[0];
        trans1[1][3] = center[1];
        trans1[2][3] = 0.0;
        for( y = 0; y < 3; y++ ) {
            for( x = 0; x < 4; x++ ) fprintf(fp, " %15.7f", trans1[y][x]);
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");

        ii++;
    }

done:
    fclose(fp);
    gCapturing = 0;
    
done0:
    free(basename);
    cleanup();
    exit(ret);
}

static void check_square( ARMarkerInfo *markerInfo, int markerNum, float *markerWidths )
{
    float   wlen, len[4], ave, sd;
    float   vec[4][2], ip1, ip2;
    int     i, j;

    for( i = 0; i < markerNum; i++ ) {
        ave = 0.0;
        for( j = 0; j < 4; j++ ) {
            vec[j][0] = markerInfo[i].vertex[(j+1)%4][0] - markerInfo[i].vertex[j][0];
            vec[j][1] = markerInfo[i].vertex[(j+1)%4][1] - markerInfo[i].vertex[j][1];
            wlen = vec[j][0] * vec[j][0] + vec[j][1] * vec[j][1];
            ave += len[j] = sqrt( wlen );
            vec[j][0] /= len[j];
            vec[j][1] /= len[j];
        }
        ave /= 4.0;
        sd = 0.0;
        for( j = 0; j < 4; j++ ) {
            sd += (len[j] - ave)*(len[j] - ave);
        }
        sd = sqrt( sd/4.0 );



        ip1 = vec[0][0]*vec[2][0] + vec[0][1]*vec[2][1];
        ip2 = vec[0][0]*vec[1][0] + vec[0][1]*vec[1][1];
        if( sd/ave > 0.01 || ip1 > -0.99 || ip2 < -0.01 || ip2 > 0.01 ) {
			markerWidths[i] = 0.0;
        }
		else {
	        markerWidths[i] = ave/dpi * 25.4;
		}
    }
}

static void pixel2mm( float px, float py, float *mx, float *my )
{
    *mx =  px          / dpi * 25.4;
    *my = (ysize - py) / dpi * 25.4;
}

/*static void mm2pixel( float mx, float my, float *px, float *py )
{
    *px = mx         * dpi / 25.4;
    *py = ysize - my * dpi / 25.4;
}*/

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
        " b             Change labeling mode.",
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
    int len, thresh, line, mode;
    AR_LABELING_THRESH_MODE threshMode;
    ARdouble tempF;
    char text[256], *text_p;
    
    glColor3ub(255, 255, 255);
    line = 1;
    
    // Image size and processing mode.
    arGetImageProcMode(gARHandle, &mode);
    if (mode == AR_IMAGE_PROC_FRAME_IMAGE) text_p = "full frame";
    else text_p = "even field only";
    snprintf(text, sizeof(text), "Processing %dx%d input image %s", xsize, ysize, text_p);
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
    arGetDebugMode(gARHandle, &mode);
    len = (int)strlen(text);
    snprintf(text + len, sizeof(text) - len, ", debug mode %s", (mode == AR_DEBUG_ENABLE ? "ON" : "OFF"));
    print(text, 2.0f,  (line - 1)*12.0f + 2.0f, 0, 1);
    line++;
    
    // Border size, image processing mode, pattern detection mode.
    arGetBorderSize(gARHandle, &tempF);
    snprintf(text, sizeof(text), "Border: %0.2f%%", tempF*100.0);
    
    // Window size.
    snprintf(text, sizeof(text), "Drawing into %dx%d window", windowWidth, windowHeight);
    print(text, 2.0f,  (line - 1)*12.0f + 2.0f, 0, 1);
    line++;
    
    // Detected markers.
    snprintf(text, sizeof(text), "Detected %d square markers. Press [space] to save multi-marker dataset.\n", detectedMarkerNum);
    print(text, 2.0f,  (line - 1)*12.0f + 2.0f, 0, 1);
    line++;
}
