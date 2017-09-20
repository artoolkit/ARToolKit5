/*
 *  calib_camera.cpp
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
#include <AR/arImageProc.h>
#include <ARUtil/time.h>


#define      CHESSBOARD_CORNER_NUM_X        7
#define      CHESSBOARD_CORNER_NUM_Y        5
#define      CHESSBOARD_PATTERN_WIDTH      30.0
#define      CALIB_IMAGE_NUM               10
#define      SAVE_FILENAME                 "camera_para.dat"
#define      SCREEN_SIZE_MARGIN             0.1


AR2VideoParamT      *gVid                 = NULL;
ARGViewportHandle   *vp;
AR_PIXEL_FORMAT      pixFormat;
int                  xsize;
int                  ysize;
ARUint8             *imageLumaCopy        = NULL;
IplImage            *calibImage           = NULL;
int                  chessboardCornerNumX = 0;
int                  chessboardCornerNumY = 0;
int                  calibImageNum        = 0;
int                  capturedImageNum     = 0;
float                patternWidth         = 0.0f;
int                  cornerFlag           = 0;
CvPoint2D32f        *corners              = NULL;
CvPoint2D32f        *cornerSet            = NULL;
static char         *cwd                  = NULL;

static void          init(int argc, char *argv[]);
static void          usage(char *com);
static void          cleanup(void);
static void          mainLoop(void);
static void          keyEvent( unsigned char key, int x, int y);
static void          calib(void);
static void          convParam(float intr[3][4], float dist[4], int xsize, int ysize, ARParam *param);
static ARdouble      getSizeFactor(ARdouble dist_factor[], int xsize, int ysize, int dist_function_version);
static void          saveParam( ARParam *param );


int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
    init(argc, argv);
    argSetDispFunc( mainLoop, 1 );
    argSetKeyFunc( keyEvent );
    ar2VideoCapStart(gVid);
    argMainLoop();
    return 0;
}

static void mainLoop(void)
{
    AR2VideoBufferT *buff;
    int             cornerCount;
    char            buf[256];
    int             i;

    buff = ar2VideoGetImage(gVid);
    if (!buff || !buff->fillFlag) {
        arUtilSleep(2);
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    argDrawMode2D(vp);
    argDrawImage(buff->buff);
  
    // Copy the luma-only image into the backing for calibImage.
    memcpy(imageLumaCopy, buff->buffLuma, xsize*ysize);
    
    cornerFlag = cvFindChessboardCorners(calibImage, cvSize(chessboardCornerNumY,chessboardCornerNumX),
                                         corners, &cornerCount, CV_CALIB_CB_ADAPTIVE_THRESH|CV_CALIB_CB_FILTER_QUADS );
    if(cornerFlag) glColor4ub(255, 0, 0, 255);
    else           glColor4ub(0, 255, 0, 255);
    glLineWidth(2.0f);
    //ARLOG("Detected corners = %d\n", cornerCount);
    for( i = 0; i < cornerCount; i++ ) {
        argDrawLineByObservedPos(corners[i].x-5, corners[i].y-5, corners[i].x+5, corners[i].y+5);
        argDrawLineByObservedPos(corners[i].x-5, corners[i].y+5, corners[i].x+5, corners[i].y-5);
        //ARLOG("  %f, %f\n", corners[i].x, corners[i].y);
        sprintf(buf, "%d\n", i);
        argDrawStringsByObservedPos(buf, corners[i].x, corners[i].y+20);
    }
    sprintf(buf, "Captured Image: %2d/%2d\n", capturedImageNum, calibImageNum);
    argDrawStringsByObservedPos(buf, 10, 30);

    argSwapBuffers();
}

static void usage(char *com)
{
    ARLOG("Usage: %s [options]\n", com);
    ARLOG("Options:\n");
    ARLOG("  --vconf <video parameter for the camera>\n");
    ARLOG("  -cornerx=n: specify the number of corners on chessboard in X direction.\n");
    ARLOG("  -cornery=n: specify the number of corners on chessboard in Y direction.\n");
    ARLOG("  -imagenum=n: specify the number of images captured for calibration.\n");
    ARLOG("  -pattwidth=n: specify the square width in the chessbaord.\n");
    ARLOG("  -h -help --help: show this message\n");
    exit(0);
}

static void init(int argc, char *argv[])
{
    ARGViewport     viewport;
    char           *vconf = NULL;
    int             i;
    int             gotTwoPartOption;
	int             screenWidth, screenHeight, screenMargin;

    chessboardCornerNumX = 0;
    chessboardCornerNumY = 0;
    calibImageNum        = 0;
    patternWidth         = 0.0f;

    arMalloc(cwd, char, MAXPATHLEN);
    if (!getcwd(cwd, MAXPATHLEN)) ARLOGe("Unable to read current working directory.\n");
    else ARLOG("Current working directory is '%s'\n", cwd);
    
    i = 1; // argv[0] is name of app, so start at 1.
    while (i < argc) {
        gotTwoPartOption = FALSE;
        // Look for two-part options first.
        if ((i + 1) < argc) {
            if (strcmp(argv[i], "--vconf") == 0) {
                i++;
                vconf = argv[i];
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
    if( patternWidth == 0.0f )       patternWidth = (float)CHESSBOARD_PATTERN_WIDTH;
    ARLOG("CHESSBOARD_CORNER_NUM_X = %d\n", chessboardCornerNumX);
    ARLOG("CHESSBOARD_CORNER_NUM_Y = %d\n", chessboardCornerNumY);
    ARLOG("CHESSBOARD_PATTERN_WIDTH = %f\n", patternWidth);
    ARLOG("CALIB_IMAGE_NUM = %d\n", calibImageNum);
    ARLOG("Video parameter: %s\n", vconf);

    if (!(gVid = ar2VideoOpen(vconf))) exit(0);
    if (ar2VideoGetSize(gVid, &xsize, &ysize) < 0) exit(0);
    ARLOG("Image size (x,y) = (%d,%d)\n", xsize, ysize);
    if ((pixFormat = ar2VideoGetPixelFormat(gVid)) == AR_PIXEL_FORMAT_INVALID) exit(0);
  
	screenWidth = glutGet(GLUT_SCREEN_WIDTH);
	screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
	if (screenWidth > 0 && screenHeight > 0) {
        screenMargin = (int)(MAX(screenWidth, screenHeight) * SCREEN_SIZE_MARGIN);
        if ((screenWidth - screenMargin) < xsize || (screenHeight - screenMargin) < ysize) {
            viewport.xsize = screenWidth - screenMargin;
            viewport.ysize = screenHeight - screenMargin;
            ARLOG("Scaling window to fit onto %dx%d screen (with %2.0f%% margin).\n", screenWidth, screenHeight, SCREEN_SIZE_MARGIN*100.0);
        } else {
            viewport.xsize = xsize;
            viewport.ysize = ysize;
        }
	} else {
        viewport.xsize = xsize;
        viewport.ysize = ysize;
	}
    viewport.sx = 0;
    viewport.sy = 0;
    if( (vp=argCreateViewport(&viewport)) == NULL ) exit(0);
    argViewportSetImageSize( vp, xsize, ysize );
    argViewportSetPixFormat( vp, pixFormat );
    argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );
    argViewportSetDistortionMode( vp, AR_GL_DISTORTION_COMPENSATE_DISABLE );
    argViewportSetDispMode(vp, AR_GL_DISP_MODE_FIT_TO_VIEWPORT_KEEP_ASPECT_RATIO);

    // Set up the grayscale image. We must always copy, since we need OpenCV to be able to wrap the memory.
    arMalloc(imageLumaCopy, ARUint8, xsize*ysize);
    calibImage = cvCreateImageHeader( cvSize(xsize, ysize), IPL_DEPTH_8U, 1);
    cvSetData(calibImage, imageLumaCopy, xsize); // Last parameter is rowBytes.
    
    // Allocate space for results.
    arMalloc(corners, CvPoint2D32f, chessboardCornerNumX*chessboardCornerNumY);
    arMalloc(cornerSet, CvPoint2D32f, chessboardCornerNumX*chessboardCornerNumY*calibImageNum);
}

static void cleanup(void)
{
    // Clean up the grayscale image.
    cvReleaseImageHeader(&calibImage);
    free(imageLumaCopy);
    
    // Free space for results.
    if (corners) {
        free(corners);
        corners = NULL;
    }
    if (cornerSet) {
        free(cornerSet);
        cornerSet = NULL;
    }
    
    ar2VideoCapStop(gVid);
    ar2VideoClose(gVid);
    argCleanup();
    
    if (cwd) {
        free(cwd);
        cwd = NULL;
    }
    
    exit(0);
}

static void  keyEvent( unsigned char key, int x, int y)
{
    CvPoint2D32f   *p1, *p2;
    int             i;

    if( key == 0x1b || key == 'q' || key == 'Q' ) {
        cleanup();
    }

    if( cornerFlag && key==' ' ) {
        cvFindCornerSubPix( calibImage, corners, chessboardCornerNumX*chessboardCornerNumY, cvSize(5,5),
                            cvSize(-1,-1), cvTermCriteria (CV_TERMCRIT_ITER, 100, 0.1)  );
        p1 = &corners[0];
        p2 = &cornerSet[capturedImageNum*chessboardCornerNumX*chessboardCornerNumY];
        for( i = 0; i < chessboardCornerNumX*chessboardCornerNumY; i++ ) {
            *(p2++) = *(p1++);
        }
        capturedImageNum++;
        ARLOG("---------- %2d/%2d -----------\n", capturedImageNum, calibImageNum);
        for( i = 0; i < chessboardCornerNumX*chessboardCornerNumY; i++ ) {
            ARLOG("  %f, %f\n", corners[i].x, corners[i].y);
        }
        ARLOG("---------- %2d/%2d -----------\n", capturedImageNum, calibImageNum);

        if( capturedImageNum == calibImageNum ) {
            calib();
            cleanup();
        }
    }
}

static void calib(void)
{
    ARParam         param;
    CvMat          *objectPoints;
    CvMat          *imagePoints;
    CvMat          *pointCounts;
    CvMat          *intrinsics;
    CvMat          *distortionCoeff;
    CvMat          *rotationVectors;
    CvMat          *translationVectors;
    CvMat          *rotationVector;
    CvMat          *rotationMatrix;
    float           intr[3][4];
    float           dist[4];
    ARdouble        trans[3][4];
    ARdouble        cx, cy, cz, hx, hy, h, sx, sy, ox, oy, err;
    int             i, j, k, l;

    objectPoints       = cvCreateMat(capturedImageNum*chessboardCornerNumX*chessboardCornerNumY, 3, CV_32FC1);
    imagePoints        = cvCreateMat(capturedImageNum*chessboardCornerNumX*chessboardCornerNumY, 2, CV_32FC1);
    pointCounts        = cvCreateMat(capturedImageNum, 1, CV_32SC1);
    intrinsics         = cvCreateMat(3, 3, CV_32FC1);
    distortionCoeff    = cvCreateMat(1, 4, CV_32FC1);
    rotationVectors    = cvCreateMat(capturedImageNum, 3, CV_32FC1);
    translationVectors = cvCreateMat(capturedImageNum, 3, CV_32FC1);
    rotationVector     = cvCreateMat(1, 3, CV_32FC1);
    rotationMatrix     = cvCreateMat(3, 3, CV_32FC1);

    l=0;
    for( k = 0; k < capturedImageNum; k++ ) {
        for( i = 0; i < chessboardCornerNumX; i++ ) {
            for( j = 0; j < chessboardCornerNumY; j++ ) {
                ((float*)(objectPoints->data.ptr + objectPoints->step*l))[0] = patternWidth*i;
                ((float*)(objectPoints->data.ptr + objectPoints->step*l))[1] = patternWidth*j;
                ((float*)(objectPoints->data.ptr + objectPoints->step*l))[2] = 0.0f;

                ((float*)(imagePoints->data.ptr + imagePoints->step*l))[0] = cornerSet[l].x;
                ((float*)(imagePoints->data.ptr + imagePoints->step*l))[1] = cornerSet[l].y;

                l++;
            }
        }
        ((int*)(pointCounts->data.ptr))[k] = chessboardCornerNumX*chessboardCornerNumY;
    }

    cvCalibrateCamera2(objectPoints, imagePoints, pointCounts, cvSize(xsize,ysize),
                       intrinsics, distortionCoeff, rotationVectors, translationVectors, 0);

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 3; i++ ) {
            intr[j][i] = ((float*)(intrinsics->data.ptr + intrinsics->step*j))[i];
        }
        intr[j][3] = 0.0f;
    }
    for( i = 0; i < 4; i++ ) {
        dist[i] = ((float*)(distortionCoeff->data.ptr))[i];
    }
    convParam(intr, dist, xsize, ysize, &param); //COVHI10434 ignored.
    arParamDisp(&param);

    l = 0;
    for( k = 0; k < capturedImageNum; k++ ) {
        for( i = 0; i < 3; i++ ) {
            ((float*)(rotationVector->data.ptr))[i] = ((float*)(rotationVectors->data.ptr + rotationVectors->step*k))[i];
        }
        cvRodrigues2( rotationVector, rotationMatrix );
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 3; i++ ) {
                trans[j][i] = ((float*)(rotationMatrix->data.ptr + rotationMatrix->step*j))[i];
            }
            trans[j][3] = ((float*)(translationVectors->data.ptr + translationVectors->step*k))[j];
        }
        //arParamDispExt(trans);

        err = 0.0;
        for( i = 0; i < chessboardCornerNumX; i++ ) {
            for( j = 0; j < chessboardCornerNumY; j++ ) {
                cx = trans[0][0] * patternWidth*i + trans[0][1] * patternWidth*j + trans[0][3];
                cy = trans[1][0] * patternWidth*i + trans[1][1] * patternWidth*j + trans[1][3];
                cz = trans[2][0] * patternWidth*i + trans[2][1] * patternWidth*j + trans[2][3];
                hx = param.mat[0][0] * cx + param.mat[0][1] * cy + param.mat[0][2] * cz + param.mat[0][3];
                hy = param.mat[1][0] * cx + param.mat[1][1] * cy + param.mat[1][2] * cz + param.mat[1][3];
                h  = param.mat[2][0] * cx + param.mat[2][1] * cy + param.mat[2][2] * cz + param.mat[2][3];
                if( h == 0.0 ) continue;
                sx = hx / h;
                sy = hy / h;
                arParamIdeal2Observ(param.dist_factor, sx, sy, &ox, &oy, param.dist_function_version);
                sx = ((float*)(imagePoints->data.ptr + imagePoints->step*l))[0];
                sy = ((float*)(imagePoints->data.ptr + imagePoints->step*l))[1];
                err += (ox - sx)*(ox - sx) + (oy - sy)*(oy - sy);
                l++;
            }
        }
        err = sqrt(err/(chessboardCornerNumX*chessboardCornerNumY));
        ARLOG("Err[%2d]: %f[pixel]\n", k+1, err);
    }
    saveParam( &param );

    cvReleaseMat(&objectPoints);
    cvReleaseMat(&imagePoints);
    cvReleaseMat(&pointCounts);
    cvReleaseMat(&intrinsics);
    cvReleaseMat(&distortionCoeff);
    cvReleaseMat(&rotationVectors);
    cvReleaseMat(&translationVectors);
    cvReleaseMat(&rotationVector);
    cvReleaseMat(&rotationMatrix);
}

static void convParam(float intr[3][4], float dist[4], int xsize, int ysize, ARParam *param)
{
    ARdouble   s;
    int      i, j;

	param->dist_function_version = 4;
    param->xsize = xsize;
    param->ysize = ysize;

    param->dist_factor[0] = (ARdouble)dist[0];     /* k1  */
    param->dist_factor[1] = (ARdouble)dist[1];     /* k2  */
    param->dist_factor[2] = (ARdouble)dist[2];     /* p1  */
    param->dist_factor[3] = (ARdouble)dist[3];     /* p2  */
    param->dist_factor[4] = (ARdouble)intr[0][0];  /* fx  */
    param->dist_factor[5] = (ARdouble)intr[1][1];  /* fy  */
    param->dist_factor[6] = (ARdouble)intr[0][2];  /* x0  */
    param->dist_factor[7] = (ARdouble)intr[1][2];  /* y0  */
    param->dist_factor[8] = (ARdouble)1.0;         /* s   */

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            param->mat[j][i] = (ARdouble)intr[j][i];
        }
    }

    s = getSizeFactor(param->dist_factor, xsize, ysize, param->dist_function_version);
    param->mat[0][0] /= s;
    param->mat[0][1] /= s;
    param->mat[1][0] /= s;
    param->mat[1][1] /= s;
    param->dist_factor[8] = s;
}

static void saveParam( ARParam *param )
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
            if( arParamSave(name, 1, param) < 0 ) {
                ARLOG("Parameter write error!!\n");
            }
            else {
                ARLOG("Saved parameter file '%s/%s'.\n", cwd, name);
            }
        }
    }
    
    // Try and save with a default name.
    if (!nameOK) {
        if( arParamSave(SAVE_FILENAME, 1, param) < 0 ) {
            ARLOG("Parameter write error!!\n");
        }
        else {
            ARLOG("Saved parameter file '%s/%s'.\n", cwd, SAVE_FILENAME);
        }
    }
    
    free(name);
    free(cwd);
}

static ARdouble getSizeFactor(ARdouble dist_factor[], int xsize, int ysize, int dist_function_version)
{
    ARdouble  ox, oy, ix, iy;
    ARdouble  olen, ilen;
    ARdouble  sf, sf1;

    sf = 100.0;

    ox = 0.0;
    oy = dist_factor[7];
    olen = dist_factor[6];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = dist_factor[6] - ix;
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = xsize;
    oy = dist_factor[7];
    olen = xsize - dist_factor[6];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = ix - dist_factor[6];
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = dist_factor[6];
    oy = 0.0;
    olen = dist_factor[7];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = dist_factor[7] - iy;
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = dist_factor[6];
    oy = ysize;
    olen = ysize - dist_factor[7];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = iy - dist_factor[7];
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }


    ox = 0.0;
    oy = 0.0;
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = dist_factor[6] - ix;
    olen = dist_factor[6];
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }
    ilen = dist_factor[7] - iy;
    olen = dist_factor[7];
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = xsize;
    oy = 0.0;
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = ix - dist_factor[6];
    olen = xsize - dist_factor[6];
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }
    ilen = dist_factor[7] - iy;
    olen = dist_factor[7];
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = 0.0;
    oy = ysize;
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = dist_factor[6] - ix;
    olen = dist_factor[6];
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }
    ilen = iy - dist_factor[7];
    olen = ysize - dist_factor[7];
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = xsize;
    oy = ysize;
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = ix - dist_factor[6];
    olen = xsize - dist_factor[6];
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }
    ilen = iy - dist_factor[7];
    olen = ysize - dist_factor[7];
    //ARLOG("Olen = %f, Ilen = %f, s = %f\n", olen, ilen, ilen / olen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    if( sf == 100.0 ) sf = 1.0;

    return sf;
}
