/*
 *  calib_stereo.c
 *  ARToolKit5
 *
 *  Camera stereo parameters calibration utility.
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
 *  Author(s): Hirokazu Kato, Philip Lamb.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#ifndef __APPLE__
#  ifdef _WIN32
#    include <windows.h>
#  endif
#  include <GL/gl.h>
#else
#  include <OpenGL/gl.h>
#endif
#include <AR/ar.h>
#include <AR/arMulti.h>
#include <AR/video.h>
#include <AR/gsub.h>
#include <ARUtil/time.h>

#define          CALIB_STEREO_MARKER_CONFIG    "../share/artoolkit-utils/Data/calibStereoMarkerConfig.dat"
#define          CPARAL_NAME                   "../share/artoolkit-utils/Data/cparaL.dat"
#define          CPARAR_NAME                   "../share/artoolkit-utils/Data/cparaR.dat"
#define          TRANSL2R_NAME                 "../share/artoolkit-utils/Data/transL2R.dat"

#if defined(ARVIDEO_INPUT_DEFAULT_V4L2)
#  define        VCONFL                        "-dev=/dev/video0 -width=640 -height=480"
#  define        VCONFR                        "-dev=/dev/video1 -width=640 -height=480"
#elif defined(ARVIDEO_INPUT_DEFAULT_1394)
#  if ARVIDEO_INPUT_1394CAM_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_MONO
#    define      VCONFR                        "-mode=640x480_MONO"
#    define      VCONFR                        "-mode=640x480_MONO"
#  elif defined(ARVIDEO_INPUT_1394CAM_USE_DRAGONFLY)
#    define      VCONFR                        "-mode=640x480_MONO_COLOR"
#    define      VCONFR                        "-mode=640x480_MONO_COLOR"
#  else
#    define      VCONFL                        "-mode=640x480_YUV411"
#    define      VCONFR                        "-mode=640x480_YUV411"
#  endif
#else
#  define        VCONFL                        ""
#  define        VCONFR                        ""
#endif 


AR2VideoParamT      *vidL;
AR2VideoParamT      *vidR;
ARParam              paramL;
ARParam              paramR;
ARParamLT           *paramLTL;
ARParamLT           *paramLTR;
ARHandle            *arHandleL;
ARHandle            *arHandleR;
AR3DHandle          *ar3DHandleL;
AR3DHandle          *ar3DHandleR;
ARGViewportHandle   *vpL;
ARGViewportHandle   *vpR;
ARMultiMarkerInfoT  *configL;
ARMultiMarkerInfoT  *configR;
int                  pixelFormatL;
int                  pixelFormatR;
ARdouble             transL2R[3][4];
int                  debugMode = 0;
int                  saveFlag = 0;



static void          usage    ( char *com );
static void          init     ( int argc, char *argv[] );
static void          keyboard ( unsigned char key, int x, int y );
static void          mouse    ( int button, int state, int x, int y );
static void          dispImage( void );
static void          cleanup  ( void );



int main( int argc, char *argv[] )
{
	glutInit(&argc, argv);
    init(argc, argv);

    argSetMouseFunc(mouse);
    argSetKeyFunc(keyboard);
    argSetDispFunc(dispImage, 1);

    ar2VideoCapStart(vidL);
    ar2VideoCapStart(vidR);
    argMainLoop();
	
	return (0);
}

static void usage( char *com )
{
    ARLOG("Usage: %s [parameters]\n", com);
    ARLOG("            -vonfL=<video parameter for the Left camera>\n");
    ARLOG("            -vonfR=<video parameter for the Right camera>\n");
    exit(0);
}

static void init(int argc, char *argv[])
{
    ARGViewport        viewport;
    ARParam            wparam;
    char               line[256], line2[256];
    char               vconfL[256], vconfR[256];
    int                xsizeL, ysizeL;
    int                xsizeR, ysizeR;
    int                i;

    strcpy( vconfL, VCONFL );
    strcpy( vconfR, VCONFR );
    for( i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "-vconfL=", 8) == 0 ) {
            strcat(vconfL, " ");
            strcat(vconfL, &argv[i][8]);
        }
        else if( strncmp(argv[i], "-vconfR=", 8) == 0 ) {
            strcat(vconfR, " ");
            strcat(vconfR, &argv[i][8]);
        }
        else {
            usage( argv[0] );
        }
    }

    if( (vidL=ar2VideoOpen(vconfL)) < 0 ) exit(0);
    if( (vidR=ar2VideoOpen(vconfR)) < 0 ) exit(0);
    if( ar2VideoGetSize(vidL, &xsizeL, &ysizeL) < 0 ) exit(0);
    if( ar2VideoGetSize(vidR, &xsizeR, &ysizeR) < 0 ) exit(0);
    if( (pixelFormatL=ar2VideoGetPixelFormat(vidL)) < 0 ) exit(0);
    if( (pixelFormatR=ar2VideoGetPixelFormat(vidR)) < 0 ) exit(0);
    ARLOG("Image size for the left camera  = (%d,%d)\n", xsizeL, ysizeL);
    ARLOG("Image size for the right camera = (%d,%d)\n", xsizeR, ysizeR);

    ARLOG("Left camera parameter [%s]: ", CPARAL_NAME);
    if (!fgets( line, 256, stdin )) exit(-1);
    if( sscanf(line, "%s", line2) != 1 ) {
        strcpy( line2, CPARAL_NAME );
    }
    if( arParamLoad(line2, 1, &wparam) < 0 ) {
        ARLOGe("Camera parameter load error !!\n");
        exit(0);  
    }
    arParamChangeSize( &wparam, xsizeL, ysizeL, &paramL );
    ARLOG("*** Camera Parameter for the left camera ***\n");
    arParamDisp( &paramL );
    if ((paramLTL = arParamLTCreate(&paramL, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
        ARLOGe("Error: arParamLTCreate.\n");
        exit(-1);
    }
    if( (arHandleL=arCreateHandle(paramLTL)) == NULL ) {
        ARLOGe("Error: arCreateHandle.\n");
        exit(0);
    }
    if( (ar3DHandleL=ar3DCreateHandle(&paramL)) == NULL ) {
        ARLOGe("Error: ar3DCreateHandle.\n");
        exit(0);
    }

    ARLOG("Right camera parameter [%s]: ", CPARAR_NAME);
    if (!fgets( line, 256, stdin )) exit(-1);
    if( sscanf(line, "%s", line2) != 1 ) {
        strcpy( line2, CPARAR_NAME );
    }
    if( arParamLoad(line2, 1, &wparam) < 0 ) {
        ARLOGe("Camera parameter load error !!\n");
        exit(0);  
    }
    arParamChangeSize( &wparam, xsizeR, ysizeR, &paramR );
    ARLOG("*** Camera Parameter for the right camera ***\n");
    arParamDisp( &paramR );
    if ((paramLTR = arParamLTCreate(&paramR, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
        ARLOGe("Error: arParamLTCreate.\n");
        exit(-1);
    }
    if( (arHandleR=arCreateHandle(paramLTR)) == NULL ) {
        ARLOGe("Error: arCreateHandle.\n");
        exit(0);
    }
    if( (ar3DHandleR=ar3DCreateHandle(&paramR)) == NULL ) {
        ARLOGe("Error: ar3DCreateHandle.\n");
        exit(0);
    }

    if((configL=arMultiReadConfigFile(CALIB_STEREO_MARKER_CONFIG, NULL)) == NULL ) {
        ARLOGe("Error: arMultiReadConfigFile.\n");
        exit(0);
    }
    if((configR=arMultiReadConfigFile(CALIB_STEREO_MARKER_CONFIG, NULL)) == NULL ) {
        ARLOGe("Error: arMultiReadConfigFile.\n");
        exit(0);
    }

    /* open the graphics window */
    if( argCreateWindow(xsizeL + xsizeR, (ysizeL > ysizeR)? ysizeL: ysizeR) < 0 ) {
        ARLOGe("Error: argCreateWindow.\n");
        exit(0);
    }
    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = xsizeL;
    viewport.ysize = ysizeL;
    if( (vpL=argCreateViewport(&viewport)) == NULL ) {
        ARLOGe("Error: argCreateViewport.\n");
        exit(0);
    }
    viewport.sx = xsizeL;
    viewport.sy = 0;
    viewport.xsize = xsizeR;
    viewport.ysize = ysizeR;
    if( (vpR=argCreateViewport(&viewport)) == NULL ) {
        ARLOGe("Error: argCreateViewport.\n");
        exit(0);
    }
    argViewportSetPixFormat( vpL, pixelFormatL );
    argViewportSetPixFormat( vpR, pixelFormatR );
    argViewportSetCparam( vpL, &paramL );
    argViewportSetCparam( vpR, &paramR );
    argViewportSetDispMethod( vpL, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );
    argViewportSetDispMethod( vpR, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );

    arSetPixelFormat( arHandleL, pixelFormatL );
    arSetPixelFormat( arHandleR, pixelFormatR );
    arSetImageProcMode( arHandleL, AR_IMAGE_PROC_FRAME_IMAGE );
    arSetImageProcMode( arHandleR, AR_IMAGE_PROC_FRAME_IMAGE );
    arSetPatternDetectionMode( arHandleL, AR_MATRIX_CODE_DETECTION );
    arSetPatternDetectionMode( arHandleR, AR_MATRIX_CODE_DETECTION );
    arSetMarkerExtractionMode( arHandleL, AR_USE_TRACKING_HISTORY_V2 );
    arSetMarkerExtractionMode( arHandleR, AR_USE_TRACKING_HISTORY_V2 );
    if( debugMode == 0 ) {
        arSetDebugMode( arHandleL, AR_DEBUG_DISABLE );
        arSetDebugMode( arHandleR, AR_DEBUG_DISABLE );
    }
    else {
        arSetDebugMode( arHandleL, AR_DEBUG_ENABLE );
        arSetDebugMode( arHandleR, AR_DEBUG_ENABLE );
    }

    return;
}

static void mouse(int button, int state, int x, int y)
{
    if( button == GLUT_LEFT_BUTTON  && state == GLUT_DOWN ) {
        saveFlag = 1;
    }
}

static void keyboard(unsigned char key, int x, int y)
{
    if( key == 'd' ) {
        debugMode = 1 - debugMode;
        if( debugMode == 0 ) {
            arSetDebugMode( arHandleL, AR_DEBUG_DISABLE );
            arSetDebugMode( arHandleR, AR_DEBUG_DISABLE );
        }
        else {
            arSetDebugMode( arHandleL, AR_DEBUG_ENABLE );
            arSetDebugMode( arHandleR, AR_DEBUG_ENABLE );
        }
    }

    argDefaultKeyFunc( key, x, y );
}

static void dispImage( void )
{
    AR2VideoBufferT *videoBuffL;
    AR2VideoBufferT *videoBuffR;
    ARUint8         *dataPtrL;
    ARUint8         *dataPtrR;
    ARMarkerInfo    *markerInfoL;
    ARMarkerInfo    *markerInfoR;
    ARdouble         errL;
    ARdouble         errR;
    ARdouble         transL2M[3][4];
    int              numL;
    int              numR;
    int              i, j;
    char             line[256], line2[256];


    videoBuffL = videoBuffR = NULL;
    for(;;) {
        if( videoBuffL == NULL || videoBuffL->fillFlag == 0 ) {
            videoBuffL = ar2VideoGetImage(vidL);
        }
        if( videoBuffR == NULL || videoBuffR->fillFlag == 0 ) {
            videoBuffR = ar2VideoGetImage(vidR);
        }
        if( videoBuffL->fillFlag && videoBuffR->fillFlag ) {
            i = ((int)videoBuffR->time.sec - (int)videoBuffL->time.sec) * 1000
              + ((int)videoBuffR->time.usec - (int)videoBuffL->time.usec) / 1000;
            if( i > 20 ) {
                videoBuffL = NULL;
                ARLOG("Time diff = %d[msec]\n", i);
            }
            else if( i < -20 ) {
                videoBuffR = NULL;
                ARLOG("Time diff = %d[msec]\n", i);
            }
            else break;
        }
        else {
            arUtilSleep(2);
        }
    }
    dataPtrL = videoBuffL->buff;
    dataPtrR = videoBuffR->buff;

    if( arDetectMarker(arHandleL, videoBuffL) < 0 ) {
        cleanup();
        exit(0);
    }
    if( arDetectMarker(arHandleR, videoBuffR) < 0 ) {
        cleanup();
        exit(0);    
    }

    if( (markerInfoL = arGetMarker(arHandleL)) != NULL ) {
        numL = arGetMarkerNum(arHandleL);
        errL = arGetTransMatMultiSquare(ar3DHandleL, markerInfoL, numL, configL);
    }
    else {
        numL = 0;
        configL->prevF = 0;
    }
    if( (markerInfoR = arGetMarker(arHandleR)) != NULL ) {
        numR = arGetMarkerNum(arHandleR);
        errR = arGetTransMatMultiSquare(ar3DHandleR, markerInfoR, numR, configR);
    }
    else {
        numR = 0;
        configR->prevF = 0;
    }
    if( configL->prevF == 0 ) ARLOG("Left: NG ");
    else                      ARLOG("Left: %f ", errL);
    if( configR->prevF == 0 ) ARLOG("Right: NG\n");
    else                      ARLOG("Right: %f\n", errR);

    if (debugMode) {
        argViewportSetPixFormat(vpL, AR_PIXEL_FORMAT_MONO); // Drawing the debug image.
        argDrawMode2D(vpL);
        argDrawImage(arHandleL->labelInfo.bwImage);
    } else {
        argViewportSetPixFormat(vpL, pixelFormatL); // Drawing the input image.
        argDrawMode2D(vpL);
        argDrawImage(dataPtrL);
    }
    if( configL->prevF ) {
        for( i = 0; i < numL; i++ ) {
            for( j = 0; j < configL->marker_num; j++ ) {
                if( configL->marker[j].visible == i ) break;
            }
            if( j < configL->marker_num ) glColor3f( 1.0f, 0.0f, 0.0f );
            else                         glColor3f( 0.0f, 1.0f, 0.0f );
            argDrawSquareByIdealPos( markerInfoL[i].vertex );
        }
    }

    if (debugMode) {
        argViewportSetPixFormat(vpR, AR_PIXEL_FORMAT_MONO); // Drawing the debug image.
        argDrawMode2D(vpR);
        argDrawImage(arHandleR->labelInfo.bwImage);
    } else {
        argViewportSetPixFormat(vpR, pixelFormatR); // Drawing the input image.
        argDrawMode2D(vpR);
        argDrawImage(dataPtrR);
    }
    if( configR->prevF ) {
        for( i = 0; i < numR; i++ ) {
            for( j = 0; j < configR->marker_num; j++ ) {
                if( configR->marker[j].visible == i ) break;
            }
            if( j < configR->marker_num ) glColor3f( 1.0f, 0.0f, 0.0f );
            else                         glColor3f( 0.0f, 1.0f, 0.0f );
            argDrawSquareByIdealPos( markerInfoR[i].vertex );
        }
    }

    argSwapBuffers();

    if( saveFlag == 1 ) {
        if( configL->prevF == 0 || configR->prevF == 0 ) {
            ARLOGe("Cannot calc parameters!!\n");
        }
        else {
            arUtilMatInv( (const ARdouble (*)[4])configL->trans, transL2M );
            arUtilMatMul( (const ARdouble (*)[4])configR->trans, (const ARdouble (*)[4])transL2M, transL2R );
            arParamDispExt( transL2R );

            printf("save filename[%s]: ", TRANSL2R_NAME);
            if (!fgets( line, 256, stdin )) exit(-1);
            if( sscanf(line, "%s", line2) != 1 ) {
                strcpy( line2, TRANSL2R_NAME );
            }
            arParamSaveExt( line2, transL2R );
            ARLOG("  Saved.\n");
        }
    }
    saveFlag = 0;
}

static void cleanup(void)
{    
    ar2VideoCapStop(vidL);
    ar2VideoCapStop(vidR);
    ar2VideoClose(vidL);
    ar2VideoClose(vidR);
    argCleanup();     
}
