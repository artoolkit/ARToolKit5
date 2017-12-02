/*
 *  calib_stereo.cpp
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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#  include <windows.h>
#  define MAXPATHLEN MAX_PATH
#  include <direct.h> // getcwd
#else
#  include <sys/param.h> // MAXPATHLEN
#  include <unistd.h> // getcwd
#endif
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include "opencv2/calib3d/calib3d_c.h"
#elif defined(__linux) || defined(_WIN32)
#  include <GL/gl.h>
#  include "opencv2/calib3d/calib3d.hpp"
#endif
#include <opencv/cv.hpp>
#include <AR/ar.h>
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/icpCore.h>
#include <AR/icpCalib.h>
#include <AR/icp.h>
#include <ARUtil/time.h>

#define          CHESSBOARD_CORNER_NUM_X        7
#define          CHESSBOARD_CORNER_NUM_Y        5
#define          CHESSBOARD_PATTERN_WIDTH      30.0
#define          CALIB_IMAGE_NUM               10
#define          SAVE_FILENAME                 "transL2R.dat"
#define          SCREEN_SIZE_MARGIN             0.1

int                  chessboardCornerNumX = 0;
int                  chessboardCornerNumY = 0;
int                  calibImageNum        = 0;
int                  capturedImageNum     = 0;
float                patternWidth         = 0.0f;
int                  cornerFlag           = 0;
CvPoint2D32f        *cornersL;
CvPoint2D32f        *cornersR;
IplImage            *calibImageL;
IplImage            *calibImageR;
ICPCalibDataT       *calibData;
ICP3DCoordT         *worldCoord;

AR2VideoParamT      *vidL;
AR2VideoParamT      *vidR;
int                  xsizeL, ysizeL;
int                  xsizeR, ysizeR;
int                  pixFormatL;
int                  pixFormatR;
ARParam              paramL;
ARParam              paramR;
ARGViewportHandle   *vpL;
ARGViewportHandle   *vpR;
static AR2VideoBufferT *gVideoBuffL = NULL;
static AR2VideoBufferT *gVideoBuffR = NULL;


static void          usage(char *com);
static void          init(int argc, char *argv[]);
static void          cleanup(void);
static void          keyEvent(unsigned char key, int x, int y);
static void          mainLoop(void);
static void          copyImage(ARUint8 *p1, ARUint8 *p2, int size, int pixFormat);
static void          saveParam(ARdouble transL2R[3][4]);
static void          calib(void);



int main( int argc, char *argv[] )
{
	glutInit(&argc, argv);
    init(argc, argv);                    
    argSetDispFunc( mainLoop, 1 );
    argSetKeyFunc( keyEvent );
    ar2VideoCapStart(vidL);
    ar2VideoCapStart(vidR);
    argMainLoop();
    return 0;
}

static void usage( char *com )
{
    ARLOG("Usage: %s [options]\n", com);
    ARLOG("  -cornerx=n: specify the number of corners on chessboard in X direction.\n");
    ARLOG("  -cornery=n: specify the number of corners on chessboard in Y direction.\n");
    ARLOG("  -imagenum=n: specify the number of images captured for calibration.\n");
    ARLOG("  -pattwidth=n: specify the square width in the chessbaord.\n");
    ARLOG("  --cparaL <camera parameter file for the Left camera>\n");
    ARLOG("  --cparaR <camera parameter file for the Right camera>\n");
    ARLOG("  -cparaL=<camera parameter file for the Left camera>\n");
    ARLOG("  -cparaR=<camera parameter file for the Right camera>\n");
    ARLOG("  --vconfL <video parameter for the Left camera>\n");
    ARLOG("  --vconfR <video parameter for the Right camera>\n");
    ARLOG("  -h -help --help: show this message\n");
    exit(0);  
}

static void init(int argc, char *argv[])
{
    char              *vconfL = NULL;
    char              *vconfR = NULL;
    char              *cparaL = NULL;
    char              *cparaR = NULL;
    char               cparaLDefault[] = "../share/artoolkit-utils/Data/cparaL.dat";
    char               cparaRDefault[] = "../share/artoolkit-utils/Data/cparaR.dat";

    ARParam            wparam;
    ARGViewport        viewport;
    int                i, j;
    int                gotTwoPartOption;
	int                screenWidth, screenHeight, screenMargin;
    double             wscalef, hscalef, scalef;

    chessboardCornerNumX = 0;
    chessboardCornerNumY = 0;
    calibImageNum        = 0;
    patternWidth         = 0.0f;

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
                cparaL = argv[i];
                gotTwoPartOption = TRUE;
            } else if (strcmp(argv[i], "--cparaR") == 0) {
                i++;
                cparaR = argv[i];
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
            } else if( strncmp(argv[i], "-cornerx=", 9) == 0 ) {
                if( sscanf(&(argv[i][9]), "%d", &chessboardCornerNumX) != 1 ) usage(argv[0]);
                if( chessboardCornerNumX <= 0 ) usage(argv[0]);
            } else if( strncmp(argv[i], "-cornery=", 9) == 0 ) {
                if( sscanf(&(argv[i][9]), "%d", &chessboardCornerNumY) != 1 ) usage(argv[0]);
                if( chessboardCornerNumY <= 0 ) usage(argv[0]);
            } else if( strncmp(argv[i], "-imagenum=", 10) == 0 ) {
                if( sscanf(&(argv[i][10]), "%d", &calibImageNum) != 1 ) usage(argv[0]);
                if( calibImageNum <= 0 ) usage(argv[0]);
            } else if( strncmp(argv[i], "-pattwidth=", 11) == 0 ) {
                if( sscanf(&(argv[i][11]), "%f", &patternWidth) != 1 ) usage(argv[0]);
                if( patternWidth <= 0 ) usage(argv[0]);
            } else if( strncmp(argv[i], "-cparaL=", 8) == 0 ) {
                cparaL = &(argv[i][8]);
            } else if( strncmp(argv[i], "-cparaR=", 8) == 0 ) {
                cparaR = &(argv[i][8]);
            } else {
                ARLOGe("Error: invalid command line argument '%s'.\n", argv[i]);
                usage(argv[0]);
            }
        }
        i++;
    }

    if( chessboardCornerNumX == 0 ) chessboardCornerNumX = CHESSBOARD_CORNER_NUM_X;
    if( chessboardCornerNumY == 0 ) chessboardCornerNumY = CHESSBOARD_CORNER_NUM_Y;
    if( calibImageNum == 0 )        calibImageNum = CALIB_IMAGE_NUM;
    if( patternWidth == 0.0f )      patternWidth = (float)CHESSBOARD_PATTERN_WIDTH;
    if (!cparaL) cparaL = cparaLDefault;
    if (!cparaR) cparaR = cparaRDefault;
    ARLOG("CHESSBOARD_CORNER_NUM_X = %d\n", chessboardCornerNumX);
    ARLOG("CHESSBOARD_CORNER_NUM_Y = %d\n", chessboardCornerNumY);
    ARLOG("CHESSBOARD_PATTERN_WIDTH = %f\n", patternWidth);
    ARLOG("CALIB_IMAGE_NUM = %d\n", calibImageNum);
    ARLOG("Video parameter Left : %s\n", vconfL);
    ARLOG("Video parameter Right: %s\n", vconfR);
    ARLOG("Camera parameter Left : %s\n", cparaL);
    ARLOG("Camera parameter Right: %s\n", cparaR);

    if( (vidL=ar2VideoOpen(vconfL)) == NULL ) {
        ARLOGe("Cannot found the first camera.\n");
        exit(0);
    }
    if( (vidR=ar2VideoOpen(vconfR)) == NULL ) {
        ARLOGe("Cannot found the second camera.\n");
        exit(0);
    }
    if( ar2VideoGetSize(vidL, &xsizeL, &ysizeL) < 0 ) exit(0);
    if( ar2VideoGetSize(vidR, &xsizeR, &ysizeR) < 0 ) exit(0);
    if( (pixFormatL=ar2VideoGetPixelFormat(vidL)) < 0 ) exit(0);
    if( (pixFormatR=ar2VideoGetPixelFormat(vidR)) < 0 ) exit(0);
    ARLOG("Image size for the left camera  = (%d,%d)\n", xsizeL, ysizeL);
    ARLOG("Image size for the right camera = (%d,%d)\n", xsizeR, ysizeR);

    if( arParamLoad(cparaL, 1, &wparam) < 0 ) {
        ARLOGe("Camera parameter load error !!   %s\n", cparaL);
        exit(0);  
    }
    arParamChangeSize( &wparam, xsizeL, ysizeL, &paramL );
    ARLOG("*** Camera Parameter for the left camera ***\n");
    arParamDisp( &paramL );
    if( arParamLoad(cparaR, 1, &wparam) < 0 ) {
        ARLOGe("Camera parameter load error !!   %s\n", cparaR);
        exit(0);  
    }
    arParamChangeSize( &wparam, xsizeR, ysizeR, &paramR );
    ARLOG("*** Camera Parameter for the right camera ***\n");
    arParamDisp( &paramR );

	screenWidth = glutGet(GLUT_SCREEN_WIDTH);
	screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
	if (screenWidth > 0 && screenHeight > 0) {
        screenMargin = (int)(MAX(screenWidth, screenHeight) * SCREEN_SIZE_MARGIN);
        if ((screenWidth - screenMargin) < (xsizeL + xsizeR) || (screenHeight - screenMargin) < MAX(ysizeL, ysizeR)) {
            wscalef = (double)(screenWidth - screenMargin) / (double)(xsizeL + xsizeR);
            hscalef = (double)(screenHeight - screenMargin) / (double)MAX(ysizeL, ysizeR);
            scalef = MIN(wscalef, hscalef);
            ARLOG("Scaling %dx%d window by %0.3f to fit onto %dx%d screen (with %2.0f%% margin).\n", xsizeL + xsizeR, MAX(ysizeL, ysizeR), scalef, screenWidth, screenHeight, SCREEN_SIZE_MARGIN*100.0);
        } else {
            scalef = 1.0;
        }
    } else {
        scalef = 1.0;
	}

    /* open the graphics window */
    if( argCreateWindow((int)((xsizeL + xsizeR)*scalef), (int)(MAX(ysizeL, ysizeR)*scalef)) < 0 ) {
        ARLOGe("Error: argCreateWindow.\n");
        exit(0);
    }
    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = (int)(xsizeL*scalef);
    viewport.ysize = (int)(ysizeL*scalef);
    if( (vpL=argCreateViewport(&viewport)) == NULL ) {
        ARLOGe("Error: argCreateViewport.\n");
        exit(0);
    }
    viewport.sx = (int)(xsizeL*scalef);
    viewport.sy = 0;
    viewport.xsize = (int)(xsizeR*scalef);
    viewport.ysize = (int)(ysizeR*scalef);
    if( (vpR=argCreateViewport(&viewport)) == NULL ) {
        ARLOGe("Error: argCreateViewport.\n");
        exit(0);
    }
    argViewportSetPixFormat( vpL, pixFormatL );
    argViewportSetPixFormat( vpR, pixFormatR );
    argViewportSetCparam( vpL, &paramL );
    argViewportSetCparam( vpR, &paramR );
    argViewportSetDispMethod( vpL, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );
    argViewportSetDispMethod( vpR, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );
    argViewportSetDispMode(vpL, AR_GL_DISP_MODE_FIT_TO_VIEWPORT_KEEP_ASPECT_RATIO);
    argViewportSetDispMode(vpR, AR_GL_DISP_MODE_FIT_TO_VIEWPORT_KEEP_ASPECT_RATIO);


    calibImageL = cvCreateImage( cvSize(xsizeL, ysizeL), IPL_DEPTH_8U, 1);
    calibImageR = cvCreateImage( cvSize(xsizeR, ysizeR), IPL_DEPTH_8U, 1);
    arMalloc(cornersL, CvPoint2D32f, chessboardCornerNumX*chessboardCornerNumY);
    arMalloc(cornersR, CvPoint2D32f, chessboardCornerNumX*chessboardCornerNumY);
    arMalloc(worldCoord, ICP3DCoordT, chessboardCornerNumX*chessboardCornerNumY);
    for( i = 0; i < chessboardCornerNumX; i++ ) {
        for( j = 0; j < chessboardCornerNumY; j++ ) {
            worldCoord[i*chessboardCornerNumY+j].x = patternWidth*i;
            worldCoord[i*chessboardCornerNumY+j].y = patternWidth*j;
            worldCoord[i*chessboardCornerNumY+j].z = 0.0;
        }
    }
    arMalloc(calibData, ICPCalibDataT, calibImageNum);
    for( i = 0; i < calibImageNum; i++ ) {
        arMalloc(calibData[i].screenCoordL, ICP2DCoordT, chessboardCornerNumX*chessboardCornerNumY);
        arMalloc(calibData[i].screenCoordR, ICP2DCoordT, chessboardCornerNumX*chessboardCornerNumY);
        calibData[i].worldCoordL = worldCoord;
        calibData[i].worldCoordR = worldCoord;
        calibData[i].numL = chessboardCornerNumX*chessboardCornerNumY;
        calibData[i].numR = chessboardCornerNumX*chessboardCornerNumY;
    }

    return;
}

static void cleanup(void)
{    
    ar2VideoCapStop(vidL);
    ar2VideoCapStop(vidR);
    ar2VideoClose(vidL);
    ar2VideoClose(vidR);
    argCleanup();     
    exit(0);
}

static void  keyEvent( unsigned char key, int x, int y)
{                   
    int             i;
            
    if( key == 0x1b || key == 'q' || key == 'Q' ) {
        cleanup();
    }
    
    if( cornerFlag && key==' ' ) {  
        cvFindCornerSubPix( calibImageL, cornersL, chessboardCornerNumX*chessboardCornerNumY, cvSize(5,5),
                            cvSize(-1,-1), cvTermCriteria(CV_TERMCRIT_ITER, 100, 0.1)  );
        for( i = 0; i < chessboardCornerNumX*chessboardCornerNumY; i++ ) {
            arParamObserv2Ideal(paramL.dist_factor, (double)cornersL[i].x, (double)cornersL[i].y,
                                &calibData[capturedImageNum].screenCoordL[i].x, &calibData[capturedImageNum].screenCoordL[i].y, paramL.dist_function_version);
        }
        cvFindCornerSubPix( calibImageR, cornersR, chessboardCornerNumX*chessboardCornerNumY, cvSize(5,5),
                            cvSize(-1,-1), cvTermCriteria(CV_TERMCRIT_ITER, 100, 0.1)  );
        for( i = 0; i < chessboardCornerNumX*chessboardCornerNumY; i++ ) {
            arParamObserv2Ideal(paramR.dist_factor, (double)cornersR[i].x, (double)cornersR[i].y,
                                &calibData[capturedImageNum].screenCoordR[i].x, &calibData[capturedImageNum].screenCoordR[i].y, paramR.dist_function_version);
        }
        ARLOG("---------- %2d/%2d -----------\n", capturedImageNum+1, calibImageNum);
        for( i = 0; i < chessboardCornerNumX*chessboardCornerNumY; i++ ) {
            ARLOG("  %f, %f  ----   %f, %f\n", calibData[capturedImageNum].screenCoordL[i].x, calibData[capturedImageNum].screenCoordL[i].y,
                                                calibData[capturedImageNum].screenCoordR[i].x, calibData[capturedImageNum].screenCoordR[i].y);
        }
        ARLOG("---------- %2d/%2d -----------\n", capturedImageNum+1, calibImageNum);
        capturedImageNum++;
    
        if( capturedImageNum == calibImageNum ) {
            calib(); 
            cleanup();
        }
    }
}   

static void mainLoop( void )
{
    AR2VideoBufferT *videoBuffL;
    AR2VideoBufferT *videoBuffR;
    ARUint8         *dataPtrL;
    ARUint8         *dataPtrR;
    int              cornerFlagL;
    int              cornerFlagR;
    int              cornerCountL;
    int              cornerCountR;
    char             buf[256];
    int              i;


    if ((videoBuffL = ar2VideoGetImage(vidL))) {
        gVideoBuffL = videoBuffL;
    }
    if ((videoBuffR = ar2VideoGetImage(vidR))) {
        gVideoBuffR = videoBuffR;
    }
    
    if (gVideoBuffL && gVideoBuffR) {
        
        // Warn about significant time differences.
        i = ((int)gVideoBuffR->time.sec -  (int)gVideoBuffL->time.sec) * 1000
          + ((int)gVideoBuffR->time.usec - (int)gVideoBuffL->time.usec) / 1000;
        if( i > 20 ) {
            ARLOG("Time diff = %d[msec]\n", i);
        } else if( i < -20 ) {
            ARLOG("Time diff = %d[msec]\n", i);
        }

        dataPtrL = gVideoBuffL->buff;
        dataPtrR = gVideoBuffR->buff;
        glClear(GL_COLOR_BUFFER_BIT);
        argDrawMode2D( vpL );
        argDrawImage( dataPtrL );
        argDrawMode2D( vpR );
        argDrawImage( dataPtrR );
        
        copyImage( dataPtrL, (ARUint8 *)calibImageL->imageData, xsizeL*ysizeL, pixFormatL );
        cornerFlagL = cvFindChessboardCorners(calibImageL, cvSize(chessboardCornerNumY,chessboardCornerNumX),
                                              cornersL, &cornerCountL, CV_CALIB_CB_ADAPTIVE_THRESH|CV_CALIB_CB_FILTER_QUADS );
        
        copyImage( dataPtrR, (ARUint8 *)calibImageR->imageData, xsizeR*ysizeR, pixFormatR );
        cornerFlagR = cvFindChessboardCorners(calibImageR, cvSize(chessboardCornerNumY,chessboardCornerNumX),
                                              cornersR, &cornerCountR, CV_CALIB_CB_ADAPTIVE_THRESH|CV_CALIB_CB_FILTER_QUADS );
        
        argDrawMode2D( vpL );
        if(cornerFlagL) glColor3f(1.0f, 0.0f, 0.0f);
        else            glColor3f(0.0f, 1.0f, 0.0f);
        glLineWidth(2.0f);
        //ARLOG("Detected corners = %d\n", cornerCount);
        for( i = 0; i < cornerCountL; i++ ) {
            argDrawLineByObservedPos(cornersL[i].x-5, cornersL[i].y-5, cornersL[i].x+5, cornersL[i].y+5);
            argDrawLineByObservedPos(cornersL[i].x-5, cornersL[i].y+5, cornersL[i].x+5, cornersL[i].y-5);
            //ARLOG("  %f, %f\n", cornersL[i].x, cornersL[i].y);
            sprintf(buf, "%d\n", i);
            argDrawStringsByObservedPos(buf, cornersL[i].x, cornersL[i].y+20);
        }
        
        argDrawMode2D( vpR );
        if(cornerFlagR) glColor3f(1.0f, 0.0f, 0.0f);
        else            glColor3f(0.0f, 1.0f, 0.0f);
        glLineWidth(2.0f);
        //ARLOG("Detected corners = %d\n", cornerCount);
        for( i = 0; i < cornerCountR; i++ ) {
            argDrawLineByObservedPos(cornersR[i].x-5, cornersR[i].y-5, cornersR[i].x+5, cornersR[i].y+5);
            argDrawLineByObservedPos(cornersR[i].x-5, cornersR[i].y+5, cornersR[i].x+5, cornersR[i].y-5);
            //ARLOG("  %f, %f\n", cornersR[i].x, cornersR[i].y);
            sprintf(buf, "%d\n", i);
            argDrawStringsByObservedPos(buf, cornersR[i].x, cornersR[i].y+20);
        }
        
        if( cornerFlagL && cornerFlagR ) {
            cornerFlag = 1;
            glColor3f(1.0f, 0.0f, 0.0f);
        }
        else {
            cornerFlag = 0;
            glColor3f(0.0f, 1.0f, 0.0f);
        }
        argDrawMode2D( vpL );
        sprintf(buf, "Captured Image: %2d/%2d\n", capturedImageNum, calibImageNum);
        argDrawStringsByIdealPos(buf, 10, 30);
        
        argSwapBuffers();
        
        gVideoBuffL = gVideoBuffR = NULL;
        
    } else arUtilSleep(2);
}

static void copyImage( ARUint8 *p1, ARUint8 *p2, int size, int pixFormat )
{
    int    i, j;

    if( pixFormat == AR_PIXEL_FORMAT_RGB || pixFormat == AR_PIXEL_FORMAT_BGR ) {
        for( i = 0; i < size; i++ ) {
            j = *(p1+0) + *(p1+1) + *(p1+2);
            *(p2++) = j/3;
            p1+=3;
        }
    }
    if( pixFormat == AR_PIXEL_FORMAT_RGBA || pixFormat == AR_PIXEL_FORMAT_BGRA ) {
        for( i = 0; i < size; i++ ) {
            j = *(p1+0) + *(p1+1) + *(p1+2);
            *(p2++) = j/3;
            p1+=4;
        }
    }
    if( pixFormat == AR_PIXEL_FORMAT_ABGR || pixFormat == AR_PIXEL_FORMAT_ARGB ) {
        for( i = 0; i < size; i++ ) {
            j = *(p1+1) + *(p1+2) + *(p1+3);
            *(p2++) = j/3;
            p1+=4;
        }
    }
    if( pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_420f) {
        for( i = 0; i < size; i++ ) {
            *(p2++) = *(p1++);
        }
    }
    if( pixFormat == AR_PIXEL_FORMAT_2vuy ) {
        for( i = 0; i < size; i++ ) {
            *(p2++) = *(p1+1);
            p1+=2;
        }
    }
    if( pixFormat == AR_PIXEL_FORMAT_yuvs ) {
        for( i = 0; i < size; i++ ) {
            *(p2++) = *(p1);
            p1+=2;
        }
    }
}

static void calib(void)
{
    //COVHI10400, COVHI10352
    ICPHandleT *icpHandleL = NULL;
    ICPHandleT *icpHandleR = NULL;
    ICPDataT    icpData;
    ARdouble    initMatXw2Xc[3][4];
    ARdouble    initTransL2R[3][4], matL[3][4], matR[3][4], invMatL[3][4];
    ARdouble    transL2R[3][4];
    ARdouble    err;
    int         i;

    if( (icpHandleL = icpCreateHandle(paramL.mat)) == NULL ) {
        ARLOG("Error!! icpCreateHandle\n");
        goto done;
    }
    icpSetBreakLoopErrorThresh( icpHandleL, 0.00001 );

    if( (icpHandleR = icpCreateHandle(paramR.mat)) == NULL ) {
        ARLOG("Error!! icpCreateHandle\n");
        goto done;
    }
    icpSetBreakLoopErrorThresh( icpHandleR, 0.00001 );

    for( i = 0; i < calibImageNum; i++ ) {
        if( icpGetInitXw2Xc_from_PlanarData(paramL.mat, calibData[i].screenCoordL, calibData[i].worldCoordL, calibData[i].numL,
            calibData[i].initMatXw2Xcl) < 0 ) {
            ARLOG("Error!! icpGetInitXw2Xc_from_PlanarData\n");
            goto done;
        }
        icpData.screenCoord = calibData[i].screenCoordL;
        icpData.worldCoord  = calibData[i].worldCoordL;
        icpData.num = calibData[i].numL;
    }

    if( icpGetInitXw2Xc_from_PlanarData(paramL.mat, calibData[0].screenCoordL, calibData[0].worldCoordL, calibData[0].numL,
                                        initMatXw2Xc) < 0 ) {
        ARLOG("Error!! icpGetInitXw2Xc_from_PlanarData\n");
        goto done;
    }
    icpData.screenCoord = calibData[0].screenCoordL;
    icpData.worldCoord  = calibData[0].worldCoordL;
    icpData.num = calibData[0].numL;
    if( icpPoint(icpHandleL, &icpData, initMatXw2Xc, matL, &err) < 0 ) {
        ARLOG("Error!! icpPoint\n");
        goto done;
    }
    if( icpGetInitXw2Xc_from_PlanarData(paramR.mat, calibData[0].screenCoordR, calibData[0].worldCoordR, calibData[0].numR,
                                        matR) < 0 ) {
        ARLOG("Error!! icpGetInitXw2Xc_from_PlanarData\n");
        goto done;
    }
    icpData.screenCoord = calibData[0].screenCoordR;
    icpData.worldCoord  = calibData[0].worldCoordR;
    icpData.num = calibData[0].numR;
    if( icpPoint(icpHandleR, &icpData, initMatXw2Xc, matR, &err) < 0 ) {
        ARLOG("Error!! icpPoint\n");
        goto done;
    }
    arUtilMatInv( matL, invMatL );
    arUtilMatMul( matR, invMatL, initTransL2R );

    if( icpCalibStereo(calibData, calibImageNum, paramL.mat, paramR.mat, initTransL2R, transL2R, &err) < 0 ) {
        ARLOG("Calibration error!!\n");
        goto done;
    }
    ARLOG("Estimated transformation matrix from Left to Right.\n");
    arParamDispExt( transL2R );

    saveParam( transL2R );

done:
    free(icpHandleL);
    free(icpHandleR);
}

static void saveParam( ARdouble transL2R[3][4] )
{
    char *name = NULL, *cwd = NULL;
    size_t len;
    int nameOK;
    
    arMalloc(name, char, MAXPATHLEN);
    arMalloc(cwd, char, MAXPATHLEN);
    if (!getcwd(cwd, MAXPATHLEN)) ARLOGe("Unable to read current working directory.\n");
    
    nameOK = 0;
    ARLOG("Filename[%s]: ", SAVE_FILENAME);
    if (fgets(name, MAXPATHLEN, stdin) != NULL) {
        
        // Trim whitespace from end of name.
        len = strlen(name);
        while (len > 0 && (name[len - 1] == '\r' || name[len - 1] == '\n' || name[len - 1] == '\t' || name[len - 1] == ' ')) {
            len--;
            name[len] = '\0';
        }
        
        if (len > 0) {
            nameOK = 1;
            if( arParamSaveExt(name, transL2R) < 0 ) {
                ARLOG("Parameter write error!!\n");
            }
            else {
                ARLOG("Saved parameter file '%s/%s'.\n", cwd, name);
            }
        }
    }
    
    // Try and save with a default name.
    if (!nameOK) {
        if( arParamSaveExt(SAVE_FILENAME, transL2R) < 0 ) {
            ARLOG("Parameter write error!!\n");
        }
        else {
            ARLOG("Saved parameter file '%s/%s'.\n", cwd, SAVE_FILENAME);
        }
    }
    
    free(name);
    free(cwd);
}
