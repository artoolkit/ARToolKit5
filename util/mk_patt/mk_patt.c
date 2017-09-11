/*
 *  mk_patt.c
 *  ARToolKit5
 *
 *  Camera calibration utility.
 *
 *  Run with "--help" parameter to see usage.
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
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#if defined(_WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/ar.h>
#include <ARUtil/time.h>
#ifdef _WIN32
#  define MAXPATHLEN MAX_PATH
#else
#  include <sys/param.h> // MAXPATHLEN
#endif

#if defined(ARVIDEO_INPUT_DEFAULT_V4L2)
#  define  VCONF  "-width=640 -height=480"
#elif defined(ARVIDEO_INPUT_DEFAULT_1394)
#  if ARVIDEO_INPUT_1394CAM_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_MONO
#    if defined(ARVIDEO_INPUT_1394CAM_USE_FLEA_XGA)
#      define  VCONF  "-mode=1024x768_MONO"
#    else
#      define  VCONF  "-mode=640x480_MONO"
#    endif
#  elif defined(ARVIDEO_INPUT_1394CAM_USE_DRAGONFLY)
#    define  VCONF  "-mode=640x480_MONO_COLOR"
#  else
#    define  VCONF  "-mode=640x480_YUV411"
#  endif
#else
#  define  VCONF  ""
#endif

static int                 pixelFormat;
static int                 pixelSize;
static ARHandle           *arHandle;
static ARParam             cparam;
static ARParamLT          *cparamLT;
static ARUint8            *saveImage;
static ARMarkerInfo       *target = NULL;
static ARGViewportHandle  *vp;
static ARGViewportHandle  *vp1;
static int                 debugMode = AR_DEBUG_DISABLE;
static ARdouble            pattRatio = (ARdouble)(AR_PATT_RATIO);
static int                 gPattSize = AR_PATT_SIZE1;



static void usage(char *com);
static void init(int argc, char *argv[]);
static void cleanup(void);
static void keyEvent( unsigned char key, int x, int y);
static void mouseEvent(int button, int state, int x, int y);
static void mainLoop(void);

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
    init(argc, argv);

    argSetDispFunc( mainLoop, 1 );
    argSetKeyFunc( keyEvent );
    argSetMouseFunc( mouseEvent );
    arVideoCapStart();
    argMainLoop();
	
	return (0);
}


static void usage(char *com)
{
    ARLOG("Usage: %s [options]\n", com);
    ARLOG("Options:\n");
    ARLOG("  --pattRatio f: Specify the proportion of the marker width/height, occupied\n");
    ARLOG("             by the marker pattern. Range (0.0 - 1.0) (not inclusive).\n");
    ARLOG("             (I.e. 1.0 - 2*borderSize). Default value is 0.5.\n");
    ARLOG("  --borderSize f: DEPRECATED specify the width of the pattern border, as a\n");
    ARLOG("             percentage of the marker width. Range (0.0 - 0.5) (not inclusive).\n");
    ARLOG("             (I.e. (1.0 - pattRatio)/2). Default value is 0.25.\n");
    ARLOG("  -border=f: Alternate syntax for --borderSize f.\n");
    ARLOG("  --cpara <camera parameter file for the camera>\n");
    ARLOG("  -cpara=<camera parameter file for the camera>\n");
    ARLOG("  --vconf <video parameter for the camera>\n");
    ARLOG("  --pattSize n: Specify the number of rows and columns in the pattern space\n");
    ARLOG("             for template (pictorial) markers.\n");
    ARLOG("             Default value 16 (required for compatibility with ARToolKit prior\n");
    ARLOG("             to version 5.2). Range is [16, %d] (inclusive).\n", AR_PATT_SIZE1_MAX);
    ARLOG("  -h -help --help: show this message\n");
    exit(0);
}

static void init(int argc, char *argv[])
{
    ARParam      wparam;
    ARGViewport  viewport;
    char        *vconf = NULL;
    char         cparaDefault[] = "../share/mk_patt/Data/camera_para.dat";
    char        *cpara = NULL;
    char         buf[MAXPATHLEN];
    int          xsize, ysize;
    int          i;
    size_t       len;
    int          gotTwoPartOption;
    int          tempI;
    float        tempF;

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
            } else if (strncmp(argv[i], "-border=", 8) == 0) {
                if (sscanf(&(argv[i][8]), "%f", &tempF) == 1 && tempF > 0.0f && tempF < 0.5f) pattRatio = (ARdouble)(1.0f - 2.0f*tempF);
                else ARLOGe("Error: argument '%s' to -border= invalid.\n", argv[i]);
            } else if (strncmp(argv[i], "-cpara=", 7) == 0) {
                cpara = &(argv[i][7]);
            } else {
                ARLOGe("Error: invalid command line argument '%s'.\n", argv[i]);
                usage(argv[0]);
            }
        }
        i++;
    }

    if (!cpara) {
        printf("Enter camera parameter filename (default: '%s'): ", cparaDefault);
        if (fgets(buf, sizeof(buf), stdin) == NULL) exit(0);
        len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) { // Trim nl/cr from end.
            buf[len - 1] = '\0';
            len--;
        }
        if (len > 0) cpara = buf;
        else cpara = cparaDefault;
    }
    if (arParamLoad(cpara, 1, &wparam) < 0) {
        ARLOGe("Parameter load error !!\n");
        exit(0);
    }

	if( arVideoOpen(vconf) < 0 ) {
		ARLOGe("Unable to open video device. Exiting.\n");
        exit(0);
	}
    if( arVideoGetSize(&xsize, &ysize) < 0 ) exit(0);
    if( (pixelFormat=arVideoGetPixelFormat()) < 0 ) exit(0);
    if( (pixelSize=arVideoGetPixelSize()) < 0 ) exit(0);
    arMalloc( saveImage, ARUint8, xsize*ysize*pixelSize );
    ARLOGi("Image size (x,y) = (%d,%d)\n", xsize, ysize);

    arParamChangeSize( &wparam, xsize, ysize, &cparam );
    arParamDisp( &cparam );
    cparamLT = arParamLTCreate( &cparam, AR_PARAM_LT_DEFAULT_OFFSET );
    if( cparamLT == NULL ) {
        ARLOGe("Error: arParamLTCreate.\n");
        exit(0);
    }

    arHandle = arCreateHandle( cparamLT );
    if( arHandle == NULL ) {
        ARLOGe("Error: arCreateHandle.\n");
        exit(0);
    }
    arSetDebugMode( arHandle, AR_DEBUG_DISABLE );
    arSetPixelFormat( arHandle, pixelFormat );
    arSetImageProcMode( arHandle, AR_IMAGE_PROC_FRAME_IMAGE );
    arSetPatternDetectionMode( arHandle, AR_TEMPLATE_MATCHING_COLOR );
    arSetMarkerExtractionMode( arHandle, AR_NOUSE_TRACKING_HISTORY );
    arSetPattRatio(arHandle, (ARdouble)pattRatio);

    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = xsize;
    viewport.ysize = ysize;
    if( (vp=argCreateViewport(&viewport)) == NULL ) exit(0);
    argViewportSetCparam( vp, &cparam );
    argViewportSetPixFormat( vp, pixelFormat );
    argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );
    argViewportSetDistortionMode( vp, AR_GL_DISTORTION_COMPENSATE_ENABLE );

    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = 128;
    viewport.ysize = 128;
    vp1 = argCreateViewport     ( &viewport );
    argViewportSetImageSize     ( vp1, gPattSize, gPattSize );
    argViewportSetPixFormat     ( vp1, AR_PIXEL_FORMAT_BGR );
    argViewportSetDispMethod    ( vp1, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );
    //argViewportSetDispMethod    ( vp1, AR_GL_DISP_METHOD_GL_DRAW_PIXELS );
    argViewportSetDistortionMode( vp1, AR_GL_DISTORTION_COMPENSATE_DISABLE );
}

static void cleanup(void)
{
    arVideoCapStop();
    arVideoClose();
    argCleanup();
}

static void   keyEvent( unsigned char key, int x, int y)
{
    int value;
    
    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        cleanup();
        exit(0);
    }

    /* change the threshold value when '-' or '+' key pressed */
    if( key == '-' ) {
        arGetLabelingThresh(arHandle, &value);
        value -= 5;
        if( value < 0 ) value = 0;
        arSetLabelingThresh( arHandle, value );
        ARLOG("thresh = %d\n", value);
    }
    if( key == '+' || key == '=') {
        arGetLabelingThresh(arHandle, &value);
        value += 5;
        if( value > 255 ) value = 255;
        arSetLabelingThresh( arHandle, value );
        ARLOG("thresh = %d\n", value);
    }
    
    if( key == 'd' ) {
        if( debugMode == AR_DEBUG_DISABLE ) {
            debugMode = AR_DEBUG_ENABLE;
            argViewportSetPixFormat(vp, AR_PIXEL_FORMAT_MONO);
        } else {
            debugMode = AR_DEBUG_DISABLE;
            argViewportSetPixFormat(vp, pixelFormat);
        }
        arSetDebugMode( arHandle, debugMode );
    }
	
	if (key == 'b') {
		if (arHandle->arLabelingMode == AR_LABELING_BLACK_REGION)
			arSetLabelingMode(arHandle, AR_LABELING_WHITE_REGION);
		else arSetLabelingMode(arHandle, AR_LABELING_BLACK_REGION);
	}
}

static void mouseEvent(int button, int state, int x, int y)
{
    char   name1[256], name2[256];

    if( button == GLUT_RIGHT_BUTTON  && state == GLUT_DOWN ) {
        cleanup();
        exit(0);
    }
    if( button == GLUT_LEFT_BUTTON  && state == GLUT_DOWN && target != NULL ) {
        printf("Enter filename: ");
        if( fgets(name1, 256, stdin) == NULL ) return;
        if( sscanf(name1, "%s", name2) != 1 ) return;
        if( arPattSave(saveImage, cparam.xsize, cparam.ysize, pixelFormat, &(cparamLT->paramLTf),
                       arHandle->arImageProcMode, target, pattRatio, gPattSize, name2) < 0 ) {
            ARLOGe("ERROR!!\n");
        }
        else {
            ARLOG("  Saved\n");
        }
    }
}

static void get_cpara(ARdouble world[4][2], ARdouble vertex[4][2], ARdouble para[3][3])
{
    ARMat   *a, *b, *c;
    int     i;
    
    a = arMatrixAlloc( 8, 8 );
    b = arMatrixAlloc( 8, 1 );
    c = arMatrixAlloc( 8, 1 );
    for( i = 0; i < 4; i++ ) {
        a->m[i*16+0]  = world[i][0];
        a->m[i*16+1]  = world[i][1];
        a->m[i*16+2]  = 1.0;
        a->m[i*16+3]  = 0.0;
        a->m[i*16+4]  = 0.0;
        a->m[i*16+5]  = 0.0;
        a->m[i*16+6]  = -world[i][0] * vertex[i][0];
        a->m[i*16+7]  = -world[i][1] * vertex[i][0];
        a->m[i*16+8]  = 0.0;
        a->m[i*16+9]  = 0.0;
        a->m[i*16+10] = 0.0;
        a->m[i*16+11] = world[i][0];
        a->m[i*16+12] = world[i][1];
        a->m[i*16+13] = 1.0;
        a->m[i*16+14] = -world[i][0] * vertex[i][1];
        a->m[i*16+15] = -world[i][1] * vertex[i][1];
        b->m[i*2+0] = vertex[i][0];
        b->m[i*2+1] = vertex[i][1];
    }
    arMatrixSelfInv( a );
    arMatrixMul( c, a, b );
    for( i = 0; i < 2; i++ ) {
        para[i][0] = c->m[i*3+0];
        para[i][1] = c->m[i*3+1];
        para[i][2] = c->m[i*3+2];
    }
    para[2][0] = c->m[2*3+0];
    para[2][1] = c->m[2*3+1];
    para[2][2] = 1.0;
    arMatrixFree( a );
    arMatrixFree( b );
    arMatrixFree( c );
}

static int getPatternVerticesFromMarkerVertices(const ARdouble vertex[4][2], ARdouble patternVertex[4][2])
{
    int i;
    ARdouble    world[4][2];
    ARdouble    local[4][2];
    ARdouble    para[3][3];
    ARdouble    d, xw, yw;
    ARdouble    pattRatio1, pattRatio2;
    
    world[0][0] = 100.0;
    world[0][1] = 100.0;
    world[1][0] = 100.0 + 10.0;
    world[1][1] = 100.0;
    world[2][0] = 100.0 + 10.0;
    world[2][1] = 100.0 + 10.0;
    world[3][0] = 100.0;
    world[3][1] = 100.0 + 10.0;
    for( i = 0; i < 4; i++ ) {
        local[i][0] = vertex[i][0];
        local[i][1] = vertex[i][1];
    }
    get_cpara(world, local, para);

    pattRatio1 = (1.0 - pattRatio)/2.0 * 10.0; // borderSize * 10.0
    pattRatio2 = 10.0*pattRatio;
    
    world[0][0] = 100.0 + pattRatio1;
    world[0][1] = 100.0 + pattRatio1;
    world[1][0] = 100.0 + pattRatio1 + pattRatio2;
    world[1][1] = 100.0 + pattRatio1;
    world[2][0] = 100.0 + pattRatio1 + pattRatio2;
    world[2][1] = 100.0 + pattRatio1 + pattRatio2;
    world[3][0] = 100.0 + pattRatio1;
    world[3][1] = 100.0 + pattRatio1 + pattRatio2;
    
    for (i = 0; i < 4; i++) {
        yw = world[i][1];
        xw = world[i][0];
        d = para[2][0]*xw + para[2][1]*yw + para[2][2];
        if (d == 0) return -1;
        patternVertex[i][0] = (para[0][0]*xw + para[0][1]*yw + para[0][2])/d;
        patternVertex[i][1] = (para[1][0]*xw + para[1][1]*yw + para[1][2])/d;
    }
    return (0);
}

static void mainLoop(void)
{
    AR2VideoBufferT *buff;
    ARMarkerInfo    *markerInfo;
    int              markerNum;
    int              areamax;
    int              i;
    ARdouble         vertex[4][2];
    ARdouble         patternVertex[4][2];
    ARUint8          pattImage[AR_PATT_SIZE1_MAX*AR_PATT_SIZE1_MAX*3]; // Buffer big enough to hold largest possible image.
    int              mode;

    buff = arVideoGetImage();
    if (!buff || !buff->fillFlag) {
        arUtilSleep(2);
        return;
    }
    memcpy(saveImage, buff->buff, cparam.xsize*cparam.ysize*pixelSize);
    
    if (arDetectMarker(arHandle, buff) < 0) {
        cleanup();
        exit(0);
    }
    markerNum = arGetMarkerNum( arHandle );
    markerInfo = arGetMarker( arHandle );
    areamax = 0;
    target = NULL;
    for( i = 0; i < markerNum; i++ ) {
        if( markerInfo[i].area > areamax ) {
            areamax = markerInfo[i].area;
            target = &(markerInfo[i]);
        }
    }
    arGetDebugMode(arHandle, &mode);
    if (mode == AR_DEBUG_ENABLE) {
        argViewportSetPixFormat(vp, AR_PIXEL_FORMAT_MONO); // Drawing the debug image.
        argDrawMode2D(vp);
        arGetImageProcMode(arHandle, &mode);
        if (mode == AR_IMAGE_PROC_FRAME_IMAGE) argDrawImage(arHandle->labelInfo.bwImage);
        else argDrawImageHalf(arHandle->labelInfo.bwImage);
    } else {
        argViewportSetPixFormat(vp, pixelFormat); // Drawing the input image.
        argDrawMode2D(vp);
        argDrawImage(buff->buff);
    }

    if( target != NULL ) {
        glLineWidth(2.0f);
        glColor3f(0.0f, 1.0f, 0.0f);
        argDrawLineByIdealPos( target->vertex[0][0], target->vertex[0][1],
                                 target->vertex[1][0], target->vertex[1][1] );
        argDrawLineByIdealPos( target->vertex[3][0], target->vertex[3][1],
                                 target->vertex[0][0], target->vertex[0][1] );
        glColor3f(1.0f, 0.0f, 0.0f);
        argDrawLineByIdealPos( target->vertex[1][0], target->vertex[1][1],
                                 target->vertex[2][0], target->vertex[2][1] );
        argDrawLineByIdealPos( target->vertex[2][0], target->vertex[2][1],
                                 target->vertex[3][0], target->vertex[3][1] );
        
        // Outline pattern space in blue.
        glColor3f(0.0f, 0.0f, 1.0f);
        getPatternVerticesFromMarkerVertices((const ARdouble (*)[2])target->vertex, patternVertex);
        argDrawLineByIdealPos(patternVertex[0][0], patternVertex[0][1], patternVertex[1][0], patternVertex[1][1]);
        argDrawLineByIdealPos(patternVertex[1][0], patternVertex[1][1], patternVertex[2][0], patternVertex[2][1]);
        argDrawLineByIdealPos(patternVertex[2][0], patternVertex[2][1], patternVertex[3][0], patternVertex[3][1]);
        argDrawLineByIdealPos(patternVertex[3][0], patternVertex[3][1], patternVertex[0][0], patternVertex[0][1]);

        for( i = 0; i < 4; i++ ) {
            vertex[i][0] = target->vertex[(i+2)%4][0];
            vertex[i][1] = target->vertex[(i+2)%4][1];
        }
        if( arPattGetImage2( AR_IMAGE_PROC_FRAME_IMAGE, AR_TEMPLATE_MATCHING_COLOR, gPattSize, gPattSize*AR_PATT_SAMPLE_FACTOR1,
                            buff->buff, cparam.xsize, cparam.ysize, pixelFormat, &(cparamLT->paramLTf),
                            vertex, pattRatio, (ARUint8 *)pattImage ) == 0 ) {
            argDrawMode2D(vp1);
            argDrawImage((ARUint8 *)pattImage);
        }
    }
    argSwapBuffers();
 
    return;
}
