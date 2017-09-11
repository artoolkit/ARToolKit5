/*
 *  whiteBalance.c
 *  ARToolKit5
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
 *  Copyright 2006-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
 
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#include <GL/gl.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/ar.h>

ARGViewportHandle  *vp;
int                 xsize, ysize;
int                 pixFormat;
int                 pixSize;
int                 max, min;

static void   init(int argc, char *argv[]);
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   mainLoop(void);
static void   draw( void );
static void   dispAvaragePixelValue(void);

main(int argc, char *argv[])
{
    init(argc, argv);

    arVideoCapStart();
    argSetDispFunc( mainLoop, 1 );
    argSetKeyFunc( keyEvent );
    argMainLoop();
}

static void   keyEvent( unsigned char key, int x, int y)
{
    int     ub, vr;

    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        cleanup();
        exit(0);
    }

    if( key == ' ' ) {
        dispAvaragePixelValue();
    }
#ifdef  ARVIDEO_INPUT_1394CAM
    if( key == '1' ) {
        arVideoGetParami( AR_VIDEO_1394_WHITE_BALANCE_UB, &ub );
        arVideoGetParami( AR_VIDEO_1394_WHITE_BALANCE_VR, &vr );
        ub--;
        if( ub < min ) ub = min;
        ARLOG("UB: %d, VR: %d\n", ub, vr);
        arVideoSetParami( AR_VIDEO_1394_WHITE_BALANCE_UB, ub );
        arVideoSetParami( AR_VIDEO_1394_WHITE_BALANCE_VR, vr );
        dispAvaragePixelValue();
    }
    if( key == '2' ) {
        arVideoGetParami( AR_VIDEO_1394_WHITE_BALANCE_UB, &ub );
        arVideoGetParami( AR_VIDEO_1394_WHITE_BALANCE_VR, &vr );
        ub++;
        if( ub > max ) ub = max;
        ARLOG("UB: %d, VR: %d\n", ub, vr);
        arVideoSetParami( AR_VIDEO_1394_WHITE_BALANCE_UB, ub );
        arVideoSetParami( AR_VIDEO_1394_WHITE_BALANCE_VR, vr );
        dispAvaragePixelValue();
    }
    if( key == '3' ) {
        arVideoGetParami( AR_VIDEO_1394_WHITE_BALANCE_UB, &ub );
        arVideoGetParami( AR_VIDEO_1394_WHITE_BALANCE_VR, &vr );
        vr--;
        if( vr < min ) vr = min;
        ARLOG("UB: %d, VR: %d\n", ub, vr);
        arVideoSetParami( AR_VIDEO_1394_WHITE_BALANCE_UB, ub );
        arVideoSetParami( AR_VIDEO_1394_WHITE_BALANCE_VR, vr );
        dispAvaragePixelValue();
    }
    if( key == '4' ) {
        arVideoGetParami( AR_VIDEO_1394_WHITE_BALANCE_UB, &ub );
        arVideoGetParami( AR_VIDEO_1394_WHITE_BALANCE_VR, &vr );
        vr++;
        if( vr > max ) vr = max;
        ARLOG("UB: %d, VR: %d\n", ub, vr);
        arVideoSetParami( AR_VIDEO_1394_WHITE_BALANCE_UB, ub );
        arVideoSetParami( AR_VIDEO_1394_WHITE_BALANCE_VR, vr );
        dispAvaragePixelValue();
    }
#endif

}

static void dispAvaragePixelValue(void)
{
    AR2VideoBufferT *buff;
    ARUint8         *p;
    int             i, j, k;
    int             r, g, b;

    while (!(buff = arVideoGetImage()) || !buff->fillFlag) {
        arUtilSleep(2);
    }
    r = g = b = k = 0;
    for( j = ysize/4; j <= ysize*3/4; j++ ) {
        p = buff->buff + xsize*j*pixSize;
        for( i = xsize/4; i <= xsize*3/4; i++ ) {
            if( pixFormat == AR_PIXEL_FORMAT_ABGR ) {
                r += *(p+3);
                g += *(p+2);
                b += *(p+1);
            }
            else if( pixFormat == AR_PIXEL_FORMAT_BGRA ) {
                r += *(p+2);
                g += *(p+1);
                b += *(p+0);
            }
            else if( pixFormat == AR_PIXEL_FORMAT_BGR ) {
                r += *(p+2);
                g += *(p+1);
                b += *(p+0);
            }
            else if( pixFormat == AR_PIXEL_FORMAT_RGBA ) {
                r += *(p+0);
                g += *(p+1);
                b += *(p+2);
            }
            else if( pixFormat == AR_PIXEL_FORMAT_RGB ) {
                r += *(p+0);
                g += *(p+1);
                b += *(p+2);
            }
            k++;
            p += pixSize;
        }
    }
    r /= k;
    g /= k;
    b /= k;

    ARLOG("(R,G,B) = %3d, %3d, %3d\n", r, g, b);
}

/* main loop */
static void mainLoop(void)
{
    AR2VideoBufferT *buff;

    /* grab a video frame */
    buff = arVideoGetImage();
    if (!buff || !buff->fillFlag) {
        arUtilSleep(2);
        return;
    }

    glDisable( GL_DEPTH_TEST );
    argDrawMode2D(vp);
    argDrawImage(buff->buff);
    argSwapBuffers();
}

static void   init(int argc, char *argv[])
{
    char            vconf[512];
    ARGViewport     viewport;
    int             i;
                                                                                
    if( argc == 1 ) vconf[0] = '\0';
    else {
        strcpy( vconf, argv[1] );
        for( i = 2; i < argc; i++ ) {strcat(vconf, " "); strcat(vconf,argv[i]);}    }
                                                                                
    /* open the video path */
    if( arVideoOpen( vconf ) < 0 ) exit(0);
    if( arVideoGetSize(&xsize, &ysize) < 0 ) exit(0);
    ARLOG("Image size (x,y) = (%d,%d)\n", xsize, ysize);
    if( (pixFormat=arVideoGetPixelFormat()) < 0 ) exit(0);
    pixSize = arUtilGetPixelSize(pixFormat);
                                                                                
    /* open the graphics window */
    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = xsize;
    viewport.ysize = ysize;
    if( (vp=argCreateViewport(&viewport)) == NULL ) exit(0);
    argViewportSetImageSize( vp, xsize, ysize );
    argViewportSetPixFormat( vp, pixFormat );
    argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_GL_DRAW_PIXELS );
    argViewportSetDispMode( vp, AR_GL_DISP_MODE_FIT_TO_VIEWPORT );
    argViewportSetDistortionMode( vp, AR_GL_DISTORTION_COMPENSATE_DISABLE );

#ifdef  ARVIDEO_INPUT_1394CAM
    arVideoSetParami( AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON, 1 );
    arVideoSetParami( AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON, 0 );
    arVideoGetParami( AR_VIDEO_1394_WHITE_BALANCE_MAX_VAL, &max );
    arVideoGetParami( AR_VIDEO_1394_WHITE_BALANCE_MIN_VAL, &min );
    ARLOG("Min<->Max: %d <--> %d\n", min, max);
#endif
}

/* cleanup function called when program exits */
static void cleanup(void)
{
    arVideoCapStop();
    arVideoClose();
    argCleanup();
}
